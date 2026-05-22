#include <stdio.h>
#include "pico/stdlib.h"
#include "video/video_lcd.h"

int main()
{
    stdio_init_all();

    lcd_init();
    lcd_clear();
    lcd_print_string("Hello, World!\n");

    while (true)
    {
        sleep_ms(1000);
    }
}
