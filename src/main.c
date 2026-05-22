#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "video/video_lcd.h"
#include "input/input_keyboard.h"
#include "storage/storage_sd.h"
#include "storage/rom_flash.h"
#include "gb/gb_core.h"

#define ROM_PATH "/roms/kaeru.gb"

// Core 1 がキーボードをポーリングし、ここへ書く。Core 0 は読むだけ。
// uint8_t の書き込みは ARM で atomic なので mutex 不要。
static volatile uint8_t g_joypad = 0xFF;

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
    // キーボードコントローラが応答するまで待機（I2C 再初期化込み）。
    // USB なし起動では KB コントローラの起動が Pico より遅れる場合がある。
    // Core 1 で実行するため Core 0（GB エミュレータ）をブロックしない。
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
        }
        g_joypad = joy;
    }
}

int main()
{
    stdio_init_all();

    lcd_init();
    lcd_clear();
    lcd_print_string("PicoCalc GB Kaeru\n\n");

    kbd_init();

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

    // SD はもう不要
    sd_unmount();

    lcd_print_string("Starting emulator...\n");
    rc = gb_core_init();
    if (rc != 0) {
        char buf[40];
        snprintf(buf, sizeof(buf), "GB init FAILED: %d\n", rc);
        lcd_print_string(buf);
        while (true) tight_loop_contents();
    }

    gb_core_set_joypad(0xFF);

    // Core 1 にキーボードポーリングをオフロード（kbd_read 内の sleep_ms が Core 0 をブロックしないよう）
    multicore_launch_core1(core1_kbd_poll);

    // GB フレームタイマー（59.727fps = 16743μs）
    static repeating_timer_t frame_timer;
    add_repeating_timer_us(-16743, frame_timer_cb, NULL, &frame_timer);

    lcd_clear();

    while (true) {
        // フレームタイマーを待ってから実行（フレームレート固定）
        while (!g_frame_tick) tight_loop_contents();
        g_frame_tick = false;

        gb_core_set_joypad(g_joypad);
        gb_core_run_frame();
        lcd_gb_frame_delta(gb_fb);
    }
}
