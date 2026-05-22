#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "video/video_lcd.h"
#include "input/input_keyboard.h"
#include "storage/storage_sd.h"
#include "storage/rom_flash.h"
#include "gb/gb_core.h"

#define ROM_PATH "/roms/kaeru.gb"

// ダブルバッファ: gb_fb (gb_core.c 定義) と交互に使う
static uint8_t gb_fb_b[GB_SCREEN_H][GB_SCREEN_W];

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
    lcd_gb_dma_init();
    lcd_clear();

    // 最初の 1 フレームを gb_fb へ描画してから DMA ループへ入る
    gb_core_run_frame();

    while (true) {
        // gb_fb を DMA 転送しながら、次フレームを gb_fb_b へエミュレート
        lcd_gb_frame_start(gb_fb);
        gb_core_set_fb(gb_fb_b);
        gb_core_run_frame();
        lcd_gb_frame_wait();

        // gb_fb_b を DMA 転送しながら、次フレームを gb_fb へエミュレート
        lcd_gb_frame_start(gb_fb_b);
        gb_core_set_fb(gb_fb);
        gb_core_run_frame();
        lcd_gb_frame_wait();
    }
}
