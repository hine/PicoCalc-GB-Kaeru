#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "video/video_lcd.h"
#include "input/input_keyboard.h"
#include "storage/storage_sd.h"

int main()
{
    stdio_init_all();

    lcd_init();
    lcd_clear();
    lcd_print_string("PicoCalc SD Test\n\n");

    kbd_init();

    // SD マウントを直接リトライ。
    // USB のみ起動時は PicoCalc 電源 ON まで失敗し続け、
    // PicoCalc 電源起動時は起動直後の不安定期を過ぎれば成功する。
    lcd_print_string("Mounting SD");
    int fr = FR_NOT_READY;
    while (fr != FR_OK) {
        sd_unmount();
        sleep_ms(500);
        fr = sd_mount();
        lcd_print_string(fr == FR_OK ? "\n" : ".");
    }
    lcd_print_string("Mount OK\n\n");

    lcd_print_string("Files in /:\n");
    char buf[512];
    if (sd_list_root(buf, sizeof(buf))) {
        lcd_print_string(buf);
    } else {
        lcd_print_string("(list failed)\n");
    }

    sd_unmount();

    lcd_print_string("\nDone.\n");
    while (true) tight_loop_contents();
}
