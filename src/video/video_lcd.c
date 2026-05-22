#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>

#include <hardware/spi.h>
#include "hardware/timer.h"
#include "pico/stdlib.h"

#include "video_lcd.h"
#include "font1.h"

static unsigned char *MainFont = (unsigned char *)font1;

static int gui_fcolour;
static int gui_bcolour;
static short current_x = 0, current_y = 0;
static short gui_font_width, gui_font_height;
static short hres = 0;
static short vres = 0;
static char s_height;
static char s_width;
unsigned char lcd_buffer[LCD_WIDTH * 3] = {0};

void __not_in_flash_func(spi_write_fast)(spi_inst_t *spi, const uint8_t *src, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        while (!spi_is_writable(spi))
            tight_loop_contents();
        spi_get_hw(spi)->dr = (uint32_t)src[i];
    }
}

void __not_in_flash_func(spi_finish)(spi_inst_t *spi) {
    while (spi_is_readable(spi))
        (void)spi_get_hw(spi)->dr;
    while (spi_get_hw(spi)->sr & SPI_SSPSR_BSY_BITS)
        tight_loop_contents();
    while (spi_is_readable(spi))
        (void)spi_get_hw(spi)->dr;
    spi_get_hw(spi)->icr = SPI_SSPICR_RORIC_BITS;
}

static void set_font(void) {
    gui_font_width  = MainFont[0];
    gui_font_height = MainFont[1];
    s_height = vres / gui_font_height;
    s_width  = hres / gui_font_width;
}

static void define_region_spi(int xstart, int ystart, int xend, int yend, int rw) {
    unsigned char coord[4];
    lcd_spi_lower_cs();
    gpio_put(Pico_LCD_DC, 0);
    hw_send_spi(&(uint8_t){ILI9341_COLADDRSET}, 1);
    gpio_put(Pico_LCD_DC, 1);
    coord[0] = xstart >> 8; coord[1] = xstart;
    coord[2] = xend >> 8;   coord[3] = xend;
    hw_send_spi(coord, 4);
    gpio_put(Pico_LCD_DC, 0);
    hw_send_spi(&(uint8_t){ILI9341_PAGEADDRSET}, 1);
    gpio_put(Pico_LCD_DC, 1);
    coord[0] = ystart >> 8; coord[1] = ystart;
    coord[2] = yend >> 8;   coord[3] = yend;
    hw_send_spi(coord, 4);
    gpio_put(Pico_LCD_DC, 0);
    if (rw)
        hw_send_spi(&(uint8_t){ILI9341_MEMORYWRITE}, 1);
    else
        hw_send_spi(&(uint8_t){ILI9341_RAMRD}, 1);
    gpio_put(Pico_LCD_DC, 1);
}

static void draw_rect_spi(int x1, int y1, int x2, int y2, int c) {
    unsigned char col[3];
    if (x1 == x2 && y1 == y2) {
        if (x1 < 0 || x1 >= hres || y1 < 0 || y1 >= vres) return;
        define_region_spi(x1, y1, x2, y2, 1);
        col[0] = (c >> 16); col[1] = (c >> 8) & 0xFF; col[2] = c & 0xFF;
        hw_send_spi(col, 3);
    } else {
        int i, t, y;
        unsigned char *p;
        if (x2 <= x1) { t = x1; x1 = x2; x2 = t; }
        if (y2 <= y1) { t = y1; y1 = y2; y2 = t; }
        if (x1 < 0) x1 = 0; if (x1 >= hres) x1 = hres - 1;
        if (x2 < 0) x2 = 0; if (x2 >= hres) x2 = hres - 1;
        if (y1 < 0) y1 = 0; if (y1 >= vres) y1 = vres - 1;
        if (y2 < 0) y2 = 0; if (y2 >= vres) y2 = vres - 1;
        define_region_spi(x1, y1, x2, y2, 1);
        i = (x2 - x1 + 1) * 3;
        p = lcd_buffer;
        col[0] = (c >> 16); col[1] = (c >> 8) & 0xFF; col[2] = c & 0xFF;
        for (t = 0; t < i; t += 3) { p[t] = col[0]; p[t+1] = col[1]; p[t+2] = col[2]; }
        for (y = y1; y <= y2; y++)
            spi_write_fast(Pico_LCD_SPI_MOD, p, i);
    }
    spi_finish(Pico_LCD_SPI_MOD);
    lcd_spi_raise_cs();
}

static void draw_bitmap_spi(int x1, int y1, int width, int height, int scale,
                             int fc, int bc, unsigned char *bitmap) {
    int i, j, k, m;
    char f[3], b[3];
    int vertCoord, horizCoord, XStart, XEnd, YEnd;

    if (x1 >= hres || y1 >= vres || x1 + width * scale < 0 || y1 + height * scale < 0) return;

    vertCoord = y1;
    if (y1 < 0) y1 = 0;
    XStart = x1; if (XStart < 0) XStart = 0;
    XEnd = x1 + (width * scale) - 1; if (XEnd >= hres) XEnd = hres - 1;
    YEnd = y1 + (height * scale) - 1; if (YEnd >= vres) YEnd = vres - 1;

    f[0] = (fc >> 16); f[1] = (fc >> 8) & 0xFF; f[2] = fc & 0xFF;
    b[0] = (bc >> 16); b[1] = (bc >> 8) & 0xFF; b[2] = bc & 0xFF;

    define_region_spi(XStart, y1, XEnd, YEnd, 1);

    for (i = 0; i < height; i++) {
        for (j = 0; j < scale; j++) {
            if (vertCoord++ < 0) continue;
            if (vertCoord > vres) { lcd_spi_raise_cs(); return; }
            horizCoord = x1;
            for (k = 0; k < width; k++) {
                for (m = 0; m < scale; m++) {
                    if (horizCoord++ < 0) continue;
                    if (horizCoord > hres) continue;
                    if ((bitmap[((i * width) + k) / 8] >>
                         (((height * width) - ((i * width) + k) - 1) % 8)) & 1)
                        hw_send_spi((uint8_t *)&f, 3);
                    else
                        hw_send_spi((uint8_t *)&b, 3);
                }
            }
        }
    }
    lcd_spi_raise_cs();
}

static void lcd_print_char(int fc, int bc, char c, int orientation) {
    unsigned char *fp = (unsigned char *)MainFont;
    int height = fp[1], width = fp[0];

    if (c >= fp[2] && c < fp[2] + fp[3]) {
        unsigned char *p = fp + 4 + (int)(((c - fp[2]) * height * width) / 8);
        draw_bitmap_spi(current_x, current_y, width, height, 1, fc, bc, p);
    } else {
        draw_rect_spi(current_x, current_y,
                      current_x + width - 1, current_y + height - 1, bc);
    }
    if (orientation == ORIENT_NORMAL) current_x += width;
}

static unsigned char scrollbuff[LCD_WIDTH * 3];

static void scroll_lcd_spi(int lines) {
    (void)scrollbuff; // スクロールは現時点では未使用
    if (lines == 0) return;
    draw_rect_spi(0, vres - lines, hres - 1, vres - 1, gui_bcolour);
}

static void display_put_c(char c) {
    if (c >= MainFont[2] && c < MainFont[2] + MainFont[3]) {
        if (current_x + gui_font_width > hres) {
            display_put_c('\r');
            display_put_c('\n');
        }
    }
    switch (c) {
        case '\b':
            current_x -= gui_font_width;
            if (current_x < 0) {
                current_y -= gui_font_height;
                if (current_y < 0) current_y = 0;
                current_x = (s_width - 1) * gui_font_width;
            }
            return;
        case '\r':
            current_x = 0;
            return;
        case '\n':
            current_x = 0;
            current_y += gui_font_height;
            if (current_y + gui_font_height >= vres) {
                scroll_lcd_spi(current_y + gui_font_height - vres);
                current_y -= (current_y + gui_font_height - vres);
            }
            return;
        case '\t':
            do { display_put_c(' '); } while ((current_x / gui_font_width) % 4);
            return;
    }
    lcd_print_char(gui_fcolour, gui_bcolour, c, ORIENT_NORMAL);
}

void lcd_print_string(char *s) {
    while (*s) display_put_c(*s++);
}

void lcd_clear(void) {
    draw_rect_spi(0, 0, hres - 1, vres - 1, BLACK);
}

void hw_read_spi(unsigned char *buff, int cnt) {
    spi_read_blocking(Pico_LCD_SPI_MOD, 0xff, buff, cnt);
}

void hw_send_spi(const unsigned char *buff, int cnt) {
    spi_write_blocking(Pico_LCD_SPI_MOD, buff, cnt);
}

unsigned char __not_in_flash_func(hw1_swap_spi)(unsigned char data_out) {
    unsigned char data_in = 0;
    spi_write_read_blocking(spi1, &data_out, &data_in, 1);
    return data_in;
}

void lcd_spi_raise_cs(void) { gpio_put(Pico_LCD_CS, 1); }
void lcd_spi_lower_cs(void) { gpio_put(Pico_LCD_CS, 0); }

void spi_write_data(unsigned char data) {
    gpio_put(Pico_LCD_DC, 1);
    lcd_spi_lower_cs();
    hw_send_spi(&data, 1);
    lcd_spi_raise_cs();
}

void spi_write_command(unsigned char data) {
    gpio_put(Pico_LCD_DC, 0);
    gpio_put(Pico_LCD_CS, 0);
    spi_write_blocking(Pico_LCD_SPI_MOD, &data, 1);
    gpio_put(Pico_LCD_CS, 1);
}

void spi_write_cd(unsigned char command, int data, ...) {
    int i;
    va_list ap;
    va_start(ap, data);
    spi_write_command(command);
    for (i = 0; i < data; i++) spi_write_data((char)va_arg(ap, int));
    va_end(ap);
}

void spi_write_data24(uint32_t data) {
    uint8_t d[3] = { data >> 16, (data >> 8) & 0xFF, data & 0xFF };
    gpio_put(Pico_LCD_DC, 1);
    gpio_put(Pico_LCD_CS, 0);
    spi_write_blocking(Pico_LCD_SPI_MOD, d, 3);
    gpio_put(Pico_LCD_CS, 1);
}

void pin_set_bit(int pin, unsigned int offset) {
    switch (offset) {
        case LATCLR:
            gpio_set_pulls(pin, false, false); gpio_pull_down(pin); gpio_put(pin, 0); return;
        case LATSET:
            gpio_set_pulls(pin, false, false); gpio_pull_up(pin); gpio_put(pin, 1); return;
        case LATINV:
            gpio_xor_mask(1 << pin); return;
        case TRISSET:
            gpio_set_dir(pin, GPIO_IN); sleep_us(2); return;
        case TRISCLR:
            gpio_set_dir(pin, GPIO_OUT); gpio_set_drive_strength(pin, GPIO_DRIVE_STRENGTH_12MA); sleep_us(2); return;
        case CNPUSET:
            gpio_set_pulls(pin, true, false); return;
        case CNPDSET:
            gpio_set_pulls(pin, false, true); return;
        case CNPUCLR:
        case CNPDCLR:
            gpio_set_pulls(pin, false, false); return;
        case ODCCLR:
            gpio_set_dir(pin, GPIO_OUT); gpio_put(pin, 0); sleep_us(2); return;
        case ODCSET:
            gpio_set_pulls(pin, true, false); gpio_set_dir(pin, GPIO_IN); sleep_us(2); return;
        case ANSELCLR:
            gpio_set_function(pin, GPIO_FUNC_SIO); gpio_set_dir(pin, GPIO_IN); return;
        default: break;
    }
}

void reset_controller(void) {
    pin_set_bit(Pico_LCD_RST, LATSET);  sleep_us(10000);
    pin_set_bit(Pico_LCD_RST, LATCLR);  sleep_us(10000);
    pin_set_bit(Pico_LCD_RST, LATSET);  sleep_us(200000);
}

static void pico_lcd_init(void) {
    reset_controller();

    hres = LCD_WIDTH;
    vres = LCD_HEIGHT;

    spi_write_command(0xE0);
    spi_write_data(0x00); spi_write_data(0x03); spi_write_data(0x09); spi_write_data(0x08);
    spi_write_data(0x16); spi_write_data(0x0A); spi_write_data(0x3F); spi_write_data(0x78);
    spi_write_data(0x4C); spi_write_data(0x09); spi_write_data(0x0A); spi_write_data(0x08);
    spi_write_data(0x16); spi_write_data(0x1A); spi_write_data(0x0F);

    spi_write_command(0xE1);
    spi_write_data(0x00); spi_write_data(0x16); spi_write_data(0x19); spi_write_data(0x03);
    spi_write_data(0x0F); spi_write_data(0x05); spi_write_data(0x32); spi_write_data(0x45);
    spi_write_data(0x46); spi_write_data(0x04); spi_write_data(0x0E); spi_write_data(0x0D);
    spi_write_data(0x35); spi_write_data(0x37); spi_write_data(0x0F);

    spi_write_command(0xC0); spi_write_data(0x17); spi_write_data(0x15);
    spi_write_command(0xC1); spi_write_data(0x41);
    spi_write_command(0xC5); spi_write_data(0x00); spi_write_data(0x12); spi_write_data(0x80);
    spi_write_command(TFT_MADCTL); spi_write_data(0x48);
    spi_write_command(0x3A); spi_write_data(0x66);
    spi_write_command(0xB0); spi_write_data(0x00);
    spi_write_command(0xB1); spi_write_data(0xA0);
    spi_write_command(TFT_INVON);
    spi_write_command(0xB4); spi_write_data(0x02);
    spi_write_command(0xB6); spi_write_data(0x02); spi_write_data(0x02); spi_write_data(0x3B);
    spi_write_command(0xB7); spi_write_data(0xC6);
    spi_write_command(0xE9); spi_write_data(0x00);
    spi_write_command(0xF7); spi_write_data(0xA9); spi_write_data(0x51); spi_write_data(0x2C); spi_write_data(0x82);

    spi_write_command(TFT_SLPOUT); sleep_ms(120);
    spi_write_command(TFT_DISPON); sleep_ms(120);

    spi_write_cd(ILI9341_MEMCONTROL, 1, ILI9341_Portrait);
}

void lcd_spi_init(void) {
    gpio_init(Pico_LCD_SCK); gpio_init(Pico_LCD_TX); gpio_init(Pico_LCD_RX);
    gpio_init(Pico_LCD_CS);  gpio_init(Pico_LCD_DC); gpio_init(Pico_LCD_RST);

    gpio_set_dir(Pico_LCD_SCK, GPIO_OUT);
    gpio_set_dir(Pico_LCD_TX,  GPIO_OUT);
    gpio_set_dir(Pico_LCD_CS,  GPIO_OUT);
    gpio_set_dir(Pico_LCD_DC,  GPIO_OUT);
    gpio_set_dir(Pico_LCD_RST, GPIO_OUT);

    spi_init(Pico_LCD_SPI_MOD, LCD_SPI_SPEED);
    gpio_set_function(Pico_LCD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(Pico_LCD_TX,  GPIO_FUNC_SPI);
    gpio_set_function(Pico_LCD_RX,  GPIO_FUNC_SPI);
    gpio_set_input_hysteresis_enabled(Pico_LCD_RX, true);

    gpio_put(Pico_LCD_CS,  1);
    gpio_put(Pico_LCD_RST, 1);
}

void lcd_init(void) {
    lcd_spi_init();
    pico_lcd_init();
    set_font();
    gui_fcolour = GREEN;
    gui_bcolour = BLACK;
}
