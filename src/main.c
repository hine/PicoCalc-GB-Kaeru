#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/mutex.h"
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
// 成功時: チップは dormant 後に ROM ブートローダから再起動する（戻らない）。
// 失敗時: scratch をクリアして戻る（呼び出し元はエラー処理する）。
// 注意: 呼び出し前に frame_timer と audio DMA IRQ を停止すること。
static void go_dormant(void) {
    powman_hw->scratch[0] = SLEEP_MAGIC;
    powman_timer_set_1khz_tick_source_lposc();
    if (!powman_timer_is_running()) powman_timer_start();
    powman_enable_alarm_wakeup_at_ms(powman_timer_get_ms() + 10000);

    powman_power_state sleep_state = powman_get_power_state();
    sleep_state = powman_power_state_with_domain_off(
        sleep_state, POWMAN_POWER_DOMAIN_SWITCHED_CORE);
    sleep_state = powman_power_state_with_domain_off(
        sleep_state, POWMAN_POWER_DOMAIN_XIP_CACHE);
    powman_power_state wake_state = powman_get_power_state();

    if (!powman_configure_wakeup_state(sleep_state, wake_state)) {
        powman_hw->scratch[0] = 0;
        return;
    }
    powman_set_power_state(sleep_state);
    __dsb();
    __wfi();
    powman_hw->scratch[0] = 0;
}

// ── Core 間共有: ジョイパッド / F キー ────────────────────────────────────────
// Core 1 が書く / Core 0 が読む。uint8_t 書き込みは ARM で atomic なので mutex 不要。
static volatile uint8_t g_joypad       = 0xFF;
static volatile uint8_t g_function_key = 0;

// ── 音声 SPSC リングバッファ ──────────────────────────────────────────────────
// Core 0 (producer): gb_core_fill_audio 後に push
// Core 1 (consumer): DMA 補充タイミングで pop → audio_pwm_submit
#define AUDIO_FIFO_CAP 2048  // ステレオペア数（2の累乗）、~62ms @32774Hz
static int16_t           g_afifo[AUDIO_FIFO_CAP * 2];
static volatile uint32_t g_afifo_w = 0;  // Core 0 のみ書く
static volatile uint32_t g_afifo_r = 0;  // Core 1 のみ書く

static void audio_fifo_push(const int16_t *src, int n_pairs) {
    uint32_t w = g_afifo_w;
    for (int i = 0; i < n_pairs; i++) {
        uint32_t nw = (w + 1) & (AUDIO_FIFO_CAP - 1);
        if (nw == g_afifo_r) break;  // 満杯: ドロップ
        g_afifo[w * 2]     = src[i * 2];
        g_afifo[w * 2 + 1] = src[i * 2 + 1];
        w = nw;
    }
    __dmb();
    g_afifo_w = w;
}

// DMA IRQ コールバック（Core 1 ISR コンテキスト）: リングバッファを直接 PWM 形式に変換
static void audio_fill_pwm(uint32_t *dst, int n) {
    uint32_t r    = g_afifo_r;
    uint32_t head = g_afifo_w;
    for (int i = 0; i < n; i++) {
        uint16_t l, rv;
        if (r == head) {
            l = rv = 2048;  // アンダーラン: 12bit 無音
        } else {
            l  = (uint16_t)((g_afifo[r * 2]     + 32768) >> 4);
            rv = (uint16_t)((g_afifo[r * 2 + 1] + 32768) >> 4);
            r  = (r + 1) & (AUDIO_FIFO_CAP - 1);
        }
        dst[i] = ((uint32_t)rv << 16) | l;
    }
    __dmb();
    g_afifo_r = r;
}

// ── LCD ダブルフレームバッファ ────────────────────────────────────────────────
// Core 0: GB エミュ → g_fb[g_fb_write] に書く → Core 1 へ渡す
// Core 1: g_fb[g_lcd_fb_idx] を LCD へ転送する
// 不変条件: g_lcd_busy==true のとき g_fb_write != g_lcd_fb_idx（異なるバッファを使用）
static uint8_t g_fb[2][GB_SCREEN_H][GB_SCREEN_W];
static volatile int  g_fb_write   = 0;    // Core 0 が書き込むバッファ番号
static volatile int  g_lcd_fb_idx = 0;    // Core 1 が描画するバッファ番号
static volatile bool g_lcd_busy   = false; // true = Core 1 が描画中（またはフレーム待ち）

// ── LCD SPI バス排他 ──────────────────────────────────────────────────────────
// Core 0（Fキー操作）と Core 1（フレーム描画）が SPI バスを同時に使わないよう保護する。
// F1 スリープ時は multicore_reset_core1() 後に mutex_init() でリセットしてから使うこと。
static mutex_t g_lcd_mutex;

// Core 0 からのステータステキストクリア依頼（Core 1 がフレーム描画後に処理）
static volatile bool g_top_text_pending = false;

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
        case KEY_UP:        case 'w': return JOYPAD_UP;
        case KEY_DOWN:      case 's': return JOYPAD_DOWN;
        case KEY_LEFT:      case 'a': return JOYPAD_LEFT;
        case KEY_RIGHT:     case 'd': return JOYPAD_RIGHT;
        case ',':           case '[': return JOYPAD_A;
        case '.':           case ']': return JOYPAD_B;
        case KEY_BACKSPACE:           return JOYPAD_START;
        case KEY_DEL:                 return JOYPAD_SELECT;
        default:                      return 0;
    }
}

// Core 1: 音声 DMA 管理 + LCD フレーム描画 + キーボードポーリング
static void core1_main(void) {
    // 音声 DMA IRQ を Core 1 に登録してから即座に開始
    audio_pwm_init();

    // KB コントローラ起動待ち（この間 DMA は silence を再生）
    sleep_ms(2000);
    kbd_wait_ready();

    uint8_t joy = 0xFF;

    while (true) {
        // ── LCD: Core 0 からフレームが届いていれば描画 ────────────────────────
        if (g_lcd_busy) {
            int idx = g_lcd_fb_idx;  // バッファ番号を先にコピー
            mutex_enter_blocking(&g_lcd_mutex);
            lcd_gb_frame_delta(g_fb[idx]);
            // Core 0 が依頼した top テキストクリアも同じ SPI トランザクション内で処理
            if (g_top_text_pending) {
                lcd_status_top_text("        ");
                g_top_text_pending = false;
            }
            mutex_exit(&g_lcd_mutex);
            __dmb();
            g_lcd_busy = false;  // Core 0 へ「描画完了」を通知
        }

        // ── キーボード: I2C ポーリング（~3ms/回） ────────────────────────────
        int pressed;
        int c = kbd_read_event(&pressed);
        if (c > 0) {
            uint8_t bit = key_to_joypad_bit(c);
            if (bit) {
                if (pressed) joy &= ~bit;
                else         joy |=  bit;
            }
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
            go_dormant();
        }
        powman_hw->scratch[0] = RESUME_MAGIC;
    }

    // ─── 通常起動フロー ──────────────────────────────────────────────────────
    stdio_init_all();
    kbd_init();

    lcd_init();
    lcd_clear();
    lcd_print_string("PicoCalc GB Kaeru\n\n");

    lcd_print_string("Mounting SD");
    sleep_ms(1000);
    int fr = FR_NOT_READY;
    while (fr != FR_OK) {
        sd_unmount();
        sleep_ms(500);
        fr = sd_mount();
        lcd_print_string(fr == FR_OK ? "\n" : ".");
    }
    lcd_print_string("Mount OK\n\n");

    int rc = rom_flash_ensure(ROM_PATH);
    if (rc != 0) {
        char buf[40];
        snprintf(buf, sizeof(buf), "Flash FAILED: %d\n", rc);
        lcd_print_string(buf);
        while (true) tight_loop_contents();
    }

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

    // GB フレームバッファ書き込み先をダブルバッファの先頭に向ける
    gb_core_set_fb(g_fb[0]);
    g_fb_write = 0;

    // LCD SPI バス排他ミューテックスを Core 1 起動前に初期化
    mutex_init(&g_lcd_mutex);

    // DMA IRQ 音声補充コールバックを Core 1 起動前に登録
    audio_pwm_set_fill_cb(audio_fill_pwm);

    // Core 1: 音声 DMA + LCD + キーボード
    multicore_launch_core1(core1_main);

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
        while (!g_frame_tick) tight_loop_contents();
        g_frame_tick = false;

        // ─── F キー処理 ──────────────────────────────────────────────────────
        uint8_t fkey = g_function_key;
        if (fkey) {
            g_function_key = 0;
            char msg[9];

            if (fkey == KEY_F1) {
                // Core 1 を停止してから LCD へ直接アクセス（mutex は不要）
                multicore_reset_core1();
                mutex_init(&g_lcd_mutex);  // Core 1 が保持していた可能性があるためリセット
                kbd_init();

                lcd_status_sd_icon(1);
                lcd_status_top_text("Sleep...");
                save_state_save(SLEEP_SLOT);
                lcd_status_sd_icon(0);
                kbd_set_backlight(0);

                cancel_repeating_timer(&frame_timer);
                audio_pwm_stop();
                go_dormant();

            } else if (fkey == KEY_F2) {
                mutex_enter_blocking(&g_lcd_mutex);
                lcd_status_sd_icon(1);
                lcd_status_top_text("Saving..");
                mutex_exit(&g_lcd_mutex);

                save_state_save(g_state_slot);

                mutex_enter_blocking(&g_lcd_mutex);
                lcd_status_sd_icon(0);
                snprintf(msg, sizeof(msg), "Saved:%d ", g_state_slot);
                lcd_status_top_text(msg);
                mutex_exit(&g_lcd_mutex);

                g_top_msg_ttl = 120;
                g_top_text_pending = false;

            } else if (fkey == KEY_F3) {
                g_state_slot = (g_state_slot + 1) % N_STATE_SLOTS;
                snprintf(msg, sizeof(msg), "Slot:%d  ", g_state_slot);
                mutex_enter_blocking(&g_lcd_mutex);
                lcd_status_top_text(msg);
                mutex_exit(&g_lcd_mutex);
                g_top_msg_ttl = 120;
                g_top_text_pending = false;

            } else if (fkey == KEY_F4) {
                mutex_enter_blocking(&g_lcd_mutex);
                lcd_status_sd_icon(1);
                lcd_status_top_text("Loading.");
                mutex_exit(&g_lcd_mutex);

                int r = save_state_load(g_state_slot);

                mutex_enter_blocking(&g_lcd_mutex);
                lcd_status_sd_icon(0);
                if (r == 0) {
                    snprintf(msg, sizeof(msg), "Load:%d  ", g_state_slot);
                    lcd_status_top_text(msg);
                    lcd_gb_frame_invalidate();  // prev_fb のみ変更、SPI アクセスなし
                } else {
                    lcd_status_top_text("No state");
                }
                mutex_exit(&g_lcd_mutex);

                g_top_msg_ttl = 120;
                g_top_text_pending = false;
            }
        }

        // トップバーメッセージの自動クリア: Core 1 に依頼（直接 SPI アクセスしない）
        if (g_top_msg_ttl > 0 && --g_top_msg_ttl == 0)
            g_top_text_pending = true;

        gb_core_set_joypad(g_joypad);
        gb_core_run_frame();  // g_fb[g_fb_write] に描画

        gb_core_fill_audio(audio_buf);
        audio_fifo_push(audio_buf, GB_AUDIO_SAMPLES);

        // Core 1 が前フレームの描画を終えていれば新フレームを渡す。
        // まだ描画中なら今フレームの LCD 更新をスキップ（フレームドロップ）。
        if (!g_lcd_busy) {
            g_lcd_fb_idx = g_fb_write;
            __dmb();
            g_lcd_busy = true;
            g_fb_write ^= 1;                      // 書き込みバッファを切り替え
            gb_core_set_fb(g_fb[g_fb_write]);     // 次フレームの書き込み先を更新
        }

        // 30 秒ごとに dirty な cart RAM を SD へ自動セーブ
        if (gb_core_is_dirty() &&
            absolute_time_diff_us(last_save, get_absolute_time()) >= AUTOSAVE_INTERVAL_US) {
            mutex_enter_blocking(&g_lcd_mutex);
            lcd_status_sd_icon(1);
            mutex_exit(&g_lcd_mutex);

            save_sram_save(SAVE_PATH, gb_core_cart_ram_ptr(), gb_core_save_size());
            gb_core_clear_dirty();
            last_save = get_absolute_time();

            mutex_enter_blocking(&g_lcd_mutex);
            lcd_status_sd_icon(0);
            mutex_exit(&g_lcd_mutex);
        }
    }
}
