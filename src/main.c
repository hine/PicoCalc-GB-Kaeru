#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "video/video_lcd.h"
#include "input/input_keyboard.h"
#include "storage/storage_sd.h"
#include "storage/rom_flash.h"
#include "gb/gb_core.h"

#define ROM_PATH "/roms/kaeru.gb"

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
    lcd_clear();

    // kbd_read() 内に sleep_ms(16) があるため 4 フレームに 1 回ポーリング
    int kbd_div = 0;
    uint8_t joypad = 0xFF;

    while (true) {
        if (++kbd_div >= 4) {
            kbd_div = 0;
            joypad = 0xFF;
            switch (kbd_read()) {
                case KEY_UP:        joypad &= ~JOYPAD_UP;     break;
                case KEY_DOWN:      joypad &= ~JOYPAD_DOWN;   break;
                case KEY_LEFT:      joypad &= ~JOYPAD_LEFT;   break;
                case KEY_RIGHT:     joypad &= ~JOYPAD_RIGHT;  break;
                case 'z': case 'Z': joypad &= ~JOYPAD_A;      break;
                case 'x': case 'X': joypad &= ~JOYPAD_B;      break;
                case KEY_ENTER:     joypad &= ~JOYPAD_START;  break;
                case KEY_BACKSPACE: joypad &= ~JOYPAD_SELECT; break;
                default: break;
            }
            gb_core_set_joypad(joypad);
        }
        gb_core_run_frame();
        lcd_gb_frame_delta(gb_fb);
    }
}
