#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
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

// Core 1 がキーボードをポーリングし、ここへ書く。Core 0 は読むだけ。
// uint8_t の書き込みは ARM で atomic なので mutex 不要。
static volatile uint8_t g_joypad = 0xFF;

// Core 1 が F キー押下時に書き込む（KEY_F2/F3/F4）。Core 0 が読んでクリアする。
static volatile uint8_t g_function_key = 0;

#define N_STATE_SLOTS 10

// GB フレームレート: 4194304Hz / 70224 clocks = 59.727fps → 16743μs
// タイマーコールバックで flag を立て、Core 0 メインループがそれを待つ。
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
    // Core 1 で実行するため Core 0（GB エミュレータ）をブロックしない。
    sleep_ms(2000);
    kbd_wait_ready();

    uint8_t joy = 0xFF;  // 状態を持続させる（毎ループリセットしない）
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
            if (pressed && (c == KEY_F2 || c == KEY_F3 || c == KEY_F4))
                g_function_key = (uint8_t)c;
        }
        g_joypad = joy;
    }
}

int main()
{
    stdio_init_all();

    // I2C1 を最初期に初期化：lcd_init() の 240ms+ ウエイト中に KB STM32 が起動するため、
    // それ以前に SCL/SDA=HIGH (idle I2C バス) を確立しておく必要がある。
    // standalone 起動では Pico と KB コントローラが同時に起動し、
    // STM32 は起動直後に自分の I2C を初期化する。この時点で pull-up が必要。
    kbd_init();

    lcd_init();
    lcd_clear();
    lcd_print_string("PicoCalc GB Kaeru\n\n");

    lcd_print_string("Mounting SD");
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

    // cart RAM をセーブファイルから読み込む（ファイルなしは初回起動として正常）
    if (gb_core_save_size() > 0) {
        int sr = save_sram_load(SAVE_PATH, gb_core_cart_ram_ptr(), gb_core_save_size());
        if (sr == 0)
            lcd_print_string("Save loaded.\n");
        else if (sr == 1)
            lcd_print_string("No save file.\n");
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
    int  g_state_slot  = 0;
    int  g_top_msg_ttl = 0;

    while (true) {
        // フレームタイマーを待ってから実行（フレームレート固定）
        while (!g_frame_tick) tight_loop_contents();
        g_frame_tick = false;

        // F キー処理（セーブステート）
        uint8_t fkey = g_function_key;
        if (fkey) {
            g_function_key = 0;
            char msg[9];
            if (fkey == KEY_F3) {
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
