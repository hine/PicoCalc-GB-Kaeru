#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "input_keyboard.h"

static uint8_t kbd_inited = 0;

void kbd_init(void) {
    i2c_init(I2C_KBD_MOD, I2C_KBD_SPEED);
    gpio_set_function(I2C_KBD_SCL, GPIO_FUNC_I2C);
    gpio_set_function(I2C_KBD_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_KBD_SCL);
    gpio_pull_up(I2C_KBD_SDA);
    kbd_inited = 1;
}

int kbd_read(void) {
    if (!kbd_inited) return -1;

    uint16_t buff = 0;
    uint8_t msg = 0x09;
    static int ctrl_held = 0;

    int ret = i2c_write_timeout_us(I2C_KBD_MOD, I2C_KBD_ADDR, &msg, 1, false, 500000);
    if (ret < 0) return -1;

    sleep_ms(16);

    ret = i2c_read_timeout_us(I2C_KBD_MOD, I2C_KBD_ADDR, (uint8_t *)&buff, 2, false, 500000);
    if (ret < 0) return -1;

    if (buff == 0) return -1;

    if (buff == 0x7E03) { ctrl_held = 0; return -1; }
    if (buff == 0x7E02) { ctrl_held = 1; return -1; }

    if ((buff & 0xFF) == 1) {
        int c = buff >> 8;
        if (c >= 'a' && c <= 'z' && ctrl_held) c = c - 'a' + 1;
        return c;
    }

    return -1;
}

int kbd_read_battery(void) {
    if (!kbd_inited) return -1;

    uint16_t buff = 0;
    uint8_t msg = 0x0B;

    int ret = i2c_write_timeout_us(I2C_KBD_MOD, I2C_KBD_ADDR, &msg, 1, false, 500000);
    if (ret < 0) return -1;

    sleep_ms(16);

    ret = i2c_read_timeout_us(I2C_KBD_MOD, I2C_KBD_ADDR, (uint8_t *)&buff, 2, false, 500000);
    if (ret < 0) return -1;

    return buff ? (int)buff : -1;
}

int kbd_set_backlight(uint8_t val) {
    if (!kbd_inited) return -1;

    uint8_t msg[2] = { 0x0A | 0x80, val };

    int ret = i2c_write_timeout_us(I2C_KBD_MOD, I2C_KBD_ADDR, msg, 2, false, 500000);
    if (ret < 0) return -1;

    uint16_t buff = 0;
    sleep_ms(16);
    i2c_read_timeout_us(I2C_KBD_MOD, I2C_KBD_ADDR, (uint8_t *)&buff, 2, false, 500000);
    return 0;
}
