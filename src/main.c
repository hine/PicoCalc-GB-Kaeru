#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/powman.h"
#include "video/video_lcd.h"
#include "input/input_keyboard.h"
#include "storage/storage_sd.h"
#include "storage/rom_flash.h"
#include "storage/save_sram.h"
#include "storage/save_state.h"
#include "audio/audio_pwm.h"
#include "gb/gb_core.h"

#define ROM_PATH  "0:/roms/kaeru.gb"
#define SAVE_PATH "0:/saves/kaeru.sav"
#define AUTOSAVE_INTERVAL_US (30 * 1000 * 1000)

// dormant スリープ状態を powman_hw->scratch[0] で管理する（AON ドメインで dormant 後も保持）
#define SLEEP_MAGIC  0xCAFEB0B0u  // dormant 中または dormant 直前
#define RESUME_MAGIC 0xFEEDBEEFu  // F1 で復帰確定 → 全初期化後にスリープステートロード
#define SLEEP_SLOT   99           // suspend 用特殊スロット（F3 の 0-9 スロットと重複しない）

// LPOSC 駆動の AON タイマーアラームで 10 秒後に起床する dormant に入る。
// switched core と XIP cache をオフにして消費電力を最小化。
// 成功時: チップは dormant 後に ROM ブートローダから再起動する（戻らない）。
// 失敗時: scratch をクリアして戻る（呼び出し元はエラー処理する）。
// 注意: 呼び出し前に frame_timer と audio DMA IRQ を停止すること。
static void go_dormant(void) {
    // save_and_disable_interrupts() は使わない。
    // PRIMASK=1 で IRQ がペンディングしていると WFI がすぐ返り、
    // powman が dormant 遷移の中途半端な状態になってシステムがクラッシュする。

    powman_hw->scratch[0] = SLEEP_MAGIC;

    // AON タイマーを LPOSC (~32kHz) で起動
    powman_timer_set_1khz_tick_source_lposc();
    if (!powman_timer_is_running()) powman_timer_start();

    // 10 秒後に起床
    powman_enable_alarm_wakeup_at_ms(powman_timer_get_ms() + 10000);

    // スリープ/復帰状態を構成
    powman_power_state sleep_state = powman_get_power_state();
    sleep_state = powman_power_state_with_domain_off(
        sleep_state, POWMAN_POWER_DOMAIN_SWITCHED_CORE);
    sleep_state = powman_power_state_with_domain_off(
        sleep_state, POWMAN_POWER_DOMAIN_XIP_CACHE);
    powman_power_state wake_state = powman_get_power_state();  // 全ドメインON

    if (!powman_configure_wakeup_state(sleep_state, wake_state)) {
        powman_hw->scratch[0] = 0;
        return;  // 設定失敗: 呼び出し元でフォールバック処理
    }

    powman_set_power_state(sleep_state);
    __dsb();
    __wfi();  // ← ここでコアが停止、dormant 開始。起床時は ROM から再起動。

    // 通常ここには到達しない（dormant に入れなかった場合のガード）
    powman_hw->scratch[0] = 0;
}

// Core 1 がキーボードをポーリングし、ここへ書く。Core 0 は読むだけ。
// uint8_t の書き込みは ARM で atomic なので mutex 不要。
static volatile uint8_t g_joypad = 0xFF;

// Core 1 が F キー押下時に書き込む（KEY_F1〜F4）。Core 0 が読んでクリアする。
static volatile uint8_t g_function_key = 0;

#define N_STATE_SLOTS 10

// GB フレームレート: 4194304Hz / 70224 clocks = 59.727fps → 16743μs
static volatile bool g_frame_tick = false;

static bool frame_timer_cb(repeating_timer_t *rt) {
    (void)rt;
    g_frame_tick = true;
    return true;
}

static uint8_t key_to_joypad_bit(int c) {
    switch (c) {
        case KEY_UP:        return JOYPAD_UP;
        case KEY_DOWN:      return JOYPAD_DOWN;
        case KEY_LEFT:      return JOYPAD_LEFT;
        case KEY_RIGHT:     return JOYPAD_RIGHT;
        case ',':           return JOYPAD_A;
        case '.':           return JOYPAD_B;
        case KEY_ENTER:     return JOYPAD_START;
        case KEY_BACKSPACE: return JOYPAD_SELECT;
        default:            return 0;
    }
}

static void core1_kbd_poll(void) {
    // standalone 起動では KB コントローラと Pico が同時に起動する。
    // KB コントローラの I2C 準備が整うまで待機してから接続を試みる。
    sleep_ms(2000);
    kbd_wait_ready();

    uint8_t joy = 0xFF;
    while (true) {
        int pressed;
        int c = kbd_read_event(&pressed);
        if (c > 0) {
            uint8_t bit = key_to_joypad_bit(c);
            if (bit) {
                if (pressed) joy &= ~bit;  // キー押下: ビットをクリア
                else         joy |=  bit;  // キー離し: ビットをセット
            }
            // F キーは押下時のみ Core 0 へ通知（1スロット、上書き可）
            if (pressed &&
                (c == KEY_F1 || c == KEY_F2 || c == KEY_F3 || c == KEY_F4))
                g_function_key = (uint8_t)c;
        }
        g_joypad = joy;
    }
}

int main()
{
    // ─── dormant 復帰チェック ────────────────────────────────────────────────
    // powman scratch[0] に SLEEP_MAGIC があれば dormant 後の起床。
    // 最小限の初期化（I2C のみ）で KEY_F1 を確認し、
    //   未押下 → go_dormant() で再び 10 秒 dormant（戻らない）
    //   押下   → RESUME_MAGIC をセットして全初期化へ fall-through
    if (powman_hw->scratch[0] == SLEEP_MAGIC) {
        powman_hw->scratch[0] = 0;
        kbd_init();
        sleep_ms(200);

        bool resume = false;
        for (int i = 0; i < 10 && !resume; i++) {
            if (kbd_read() == KEY_F1) resume = true;
            else sleep_ms(50);
        }

        if (!resume) {
            go_dormant();  // 戻らない
        }
        powman_hw->scratch[0] = RESUME_MAGIC;
    }

    // ─── 通常起動フロー ──────────────────────────────────────────────────────
    stdio_init_all();

    // I2C1 を最初期に初期化: lcd_init() の 240ms+ ウエイト中に KB STM32 が起動するため、
    // それ以前に SCL/SDA=HIGH (idle I2C バス) を確立しておく必要がある。
    kbd_init();

    lcd_init();
    lcd_clear();
    lcd_print_string("PicoCalc GB Kaeru\n\n");

    lcd_print_string("Mounting SD");
    // PicoCalc の SD/KB は遅延起動する。旧 lcd_init() の ~240ms RST 待ちがバッファ役を
    // 果たしていたが LovyanGFX は ~20ms で完了するため、明示的に待機する。
    sleep_ms(1000);
    int fr = FR_NOT_READY;
    while (fr != FR_OK) {
        sd_unmount();
        sleep_ms(500);
        fr = sd_mount();
        lcd_print_string(fr == FR_OK ? "\n" : ".");
    }
    lcd_print_string("Mount OK\n\n");

    // ROM をフラッシュへ（変更がなければスキップ）
    int rc = rom_flash_ensure(ROM_PATH);
    if (rc != 0) {
        char buf[40];
        snprintf(buf, sizeof(buf), "Flash FAILED: %d\n", rc);
        lcd_print_string(buf);
        while (true) tight_loop_contents();
    }

    audio_pwm_init();

    lcd_print_string("Starting emulator...\n");
    rc = gb_core_init();
    if (rc != 0) {
        char buf[40];
        snprintf(buf, sizeof(buf), "GB init FAILED: %d\n", rc);
        lcd_print_string(buf);
        while (true) tight_loop_contents();
    }

    // ─── セーブ / スリープ復帰ロード ─────────────────────────────────────────
    bool is_resume = (powman_hw->scratch[0] == RESUME_MAGIC);
    if (is_resume) {
        powman_hw->scratch[0] = 0;
        lcd_print_string("Resuming...\n");
        if (save_state_load(SLEEP_SLOT) == 0)
            lcd_gb_frame_invalidate();
    } else {
        if (gb_core_save_size() > 0) {
            int sr = save_sram_load(SAVE_PATH, gb_core_cart_ram_ptr(), gb_core_save_size());
            if (sr == 0)      lcd_print_string("Save loaded.\n");
            else if (sr == 1) lcd_print_string("No save file.\n");
        }
    }

    gb_core_set_joypad(0xFF);

    // Core 1 にキーボードポーリングをオフロード（kbd_read 内の sleep_ms が Core 0 をブロックしないよう）
    multicore_launch_core1(core1_kbd_poll);

    // GB フレームタイマー（59.727fps = 16743μs）
    static repeating_timer_t frame_timer;
    add_repeating_timer_us(-16743, frame_timer_cb, NULL, &frame_timer);

    lcd_clear();
    lcd_status_draw_hints();

    absolute_time_t last_save = get_absolute_time();
    static int16_t audio_buf[GB_AUDIO_SAMPLES_TOTAL];
    int g_state_slot  = 0;
    int g_top_msg_ttl = 0;

    while (true) {
        // フレームタイマーを待ってから実行（フレームレート固定）
        while (!g_frame_tick) tight_loop_contents();
        g_frame_tick = false;

        // ─── F キー処理 ──────────────────────────────────────────────────────
        uint8_t fkey = g_function_key;
        if (fkey) {
            g_function_key = 0;
            char msg[9];
            if (fkey == KEY_F1) {
                // スリープ: ゲーム状態を保存して dormant へ（戻らない）
                // Core 1 を先に停止: kbd_set_backlight と I2C1 が競合するのを防ぐ
                multicore_reset_core1();
                kbd_init();  // Core 1 が途中だった I2C1 トランザクションをリセット

                lcd_status_sd_icon(1);
                lcd_status_top_text("Sleep...");
                save_state_save(SLEEP_SLOT);  // SD(SPI0) - I2C と競合しない
                lcd_status_sd_icon(0);
                kbd_set_backlight(0);  // Core 1 停止済みなので I2C1 安全

                // dormant 前に周期 IRQ を停止（ペンディング IRQ が WFI を妨げる）
                cancel_repeating_timer(&frame_timer);
                audio_pwm_stop();

                go_dormant();
            } else if (fkey == KEY_F3) {
                g_state_slot = (g_state_slot + 1) % N_STATE_SLOTS;
                snprintf(msg, sizeof(msg), "Slot:%d  ", g_state_slot);
                lcd_status_top_text(msg);
                g_top_msg_ttl = 120;
            } else if (fkey == KEY_F2) {
                lcd_status_sd_icon(1);
                lcd_status_top_text("Saving..");
                save_state_save(g_state_slot);
                lcd_status_sd_icon(0);
                snprintf(msg, sizeof(msg), "Saved:%d ", g_state_slot);
                lcd_status_top_text(msg);
                g_top_msg_ttl = 120;
            } else if (fkey == KEY_F4) {
                lcd_status_sd_icon(1);
                lcd_status_top_text("Loading.");
                int r = save_state_load(g_state_slot);
                lcd_status_sd_icon(0);
                if (r == 0) {
                    snprintf(msg, sizeof(msg), "Load:%d  ", g_state_slot);
                    lcd_status_top_text(msg);
                    lcd_gb_frame_invalidate();
                } else {
                    lcd_status_top_text("No state");
                }
                g_top_msg_ttl = 120;
            }
        }

        // トップバーメッセージの自動クリア
        if (g_top_msg_ttl > 0 && --g_top_msg_ttl == 0)
            lcd_status_top_text("        ");

        gb_core_set_joypad(g_joypad);
        gb_core_run_frame();

        gb_core_fill_audio(audio_buf);
        audio_pwm_submit(audio_buf, GB_AUDIO_SAMPLES);

        lcd_gb_frame_delta(gb_fb);

        // 30 秒ごとに dirty な cart RAM を SD へ自動セーブ
        if (gb_core_is_dirty() &&
            absolute_time_diff_us(last_save, get_absolute_time()) >= AUTOSAVE_INTERVAL_US) {
            lcd_status_sd_icon(1);
            save_sram_save(SAVE_PATH, gb_core_cart_ram_ptr(), gb_core_save_size());
            gb_core_clear_dirty();
            last_save = get_absolute_time();
            lcd_status_sd_icon(0);
        }
    }
}
