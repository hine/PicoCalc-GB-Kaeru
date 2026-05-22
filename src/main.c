#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "video/video_lcd.h"
#include "input/input_keyboard.h"

static const char *key_name(int c) {
    switch (c) {
        case KEY_UP:    return "[UP]";
        case KEY_DOWN:  return "[DOWN]";
        case KEY_LEFT:  return "[LEFT]";
        case KEY_RIGHT: return "[RIGHT]";
        case KEY_ESC:   return "[ESC]";
        case KEY_ENTER: return "[ENTER]";
        case KEY_BACKSPACE: return "[BS]";
        case KEY_F1:    return "[F1]";
        case KEY_F2:    return "[F2]";
        case KEY_F3:    return "[F3]";
        case KEY_F4:    return "[F4]";
        case KEY_F5:    return "[F5]";
        default:        return NULL;
    }
}

int main()
{
    stdio_init_all();

    lcd_init();
    lcd_clear();
    lcd_print_string("PicoCalc KB Test\n");
    lcd_print_string("Press any key...\n\n");

    kbd_init();

    while (true) {
        int c = kbd_read();
        if (c < 0) continue;

        const char *name = key_name(c);
        if (name) {
            lcd_print_string(name);
            lcd_print_string("\n");
        } else if (c >= 0x20 && c < 0x80) {
            char buf[2] = { (char)c, '\0' };
            lcd_print_string(buf);
        }
    }
}
