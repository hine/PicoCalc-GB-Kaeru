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

static void core1_kbd_poll(void) {
    while (true) {
        uint8_t joy = 0xFF;
        switch (kbd_read()) {
            case KEY_UP:        joy &= ~JOYPAD_UP;     break;
            case KEY_DOWN:      joy &= ~JOYPAD_DOWN;   break;
            case KEY_LEFT:      joy &= ~JOYPAD_LEFT;   break;
            case KEY_RIGHT:     joy &= ~JOYPAD_RIGHT;  break;
            case 'z': case 'Z': joy &= ~JOYPAD_A;      break;
            case 'x': case 'X': joy &= ~JOYPAD_B;      break;
            case KEY_ENTER:     joy &= ~JOYPAD_START;  break;
            case KEY_BACKSPACE: joy &= ~JOYPAD_SELECT; break;
            default: break;
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

    // Core 1 にキーボードポーリングをオフロード（kbd_read 内の sleep_ms(16) が Core 0 をブロックしないよう）
    multicore_launch_core1(core1_kbd_poll);

    lcd_clear();

    while (true) {
        gb_core_set_joypad(g_joypad);
        gb_core_run_frame();
        lcd_gb_frame_delta(gb_fb);
    }
}
