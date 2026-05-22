#ifndef INPUT_KEYBOARD_H
#define INPUT_KEYBOARD_H

#include <stdint.h>

#define I2C_KBD_MOD   i2c1
#define I2C_KBD_SDA   6
#define I2C_KBD_SCL   7
#define I2C_KBD_SPEED 10000
#define I2C_KBD_ADDR  0x1F

// 特殊キーコード
#define KEY_BACKSPACE 0x08
#define KEY_ENTER     0x0A
#define KEY_ESC       0xB1
#define KEY_UP        0xB5
#define KEY_DOWN      0xB6
#define KEY_LEFT      0xB4
#define KEY_RIGHT     0xB7
#define KEY_F1        0x81
#define KEY_F2        0x82
#define KEY_F3        0x83
#define KEY_F4        0x84
#define KEY_F5        0x85
#define KEY_F6        0x86
#define KEY_F7        0x87
#define KEY_F8        0x88
#define KEY_F9        0x89
#define KEY_F10       0x8A
#define KEY_POWER     0x91

void kbd_init(void);
int  kbd_read(void);          // 押下キーの ASCII を返す。なければ -1
int  kbd_read_battery(void);  // バッテリー残量
int  kbd_set_backlight(uint8_t val);

#endif // INPUT_KEYBOARD_H
