#ifndef VIDEO_LCD_H
#define VIDEO_LCD_H

#include <hardware/spi.h>

#define LCD_SPI_SPEED   50000000

#define Pico_LCD_SCK 10
#define Pico_LCD_TX  11
#define Pico_LCD_RX  12
#define Pico_LCD_CS  13
#define Pico_LCD_DC  14
#define Pico_LCD_RST 15

#define ILI9488  1
#ifdef ILI9488
#define LCD_WIDTH  320
#define LCD_HEIGHT 320
#endif

#define TFT_SLPOUT 0x11
#define TFT_INVOFF 0x20
#define TFT_INVON  0x21
#define TFT_DISPOFF 0x28
#define TFT_DISPON  0x29
#define TFT_MADCTL  0x36

#define ILI9341_MEMCONTROL      0x36
#define ILI9341_MADCTL_MX       0x40
#define ILI9341_MADCTL_BGR      0x08
#define ILI9341_COLADDRSET      0x2A
#define ILI9341_PAGEADDRSET     0x2B
#define ILI9341_MEMORYWRITE     0x2C
#define ILI9341_RAMRD           0x2E
#define ILI9341_Portrait        (ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR)

#define ORIENT_NORMAL 0

#define RGB(r, g, b) (unsigned int)(((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF))
#define WHITE   RGB(255, 255, 255)
#define BLACK   RGB(0,   0,   0)
#define RED     RGB(255, 0,   0)
#define GREEN   RGB(0,   255, 0)
#define BLUE    RGB(0,   0,   255)
#define YELLOW  RGB(255, 255, 0)
#define CYAN    RGB(0,   255, 255)
#define GRAY    RGB(128, 128, 128)

#define Pico_LCD_SPI_MOD spi1
#define nop asm("NOP")

#define LATCLR   5
#define LATSET   6
#define TRISSET  -2
#define TRISCLR  -3
#define CNPUSET  14
#define CNPDSET  18
#define CNPUCLR  13
#define CNPDCLR  17
#define ODCCLR   9
#define ODCSET   10
#define LATINV   7
#define ANSELCLR -7

extern void __not_in_flash_func(spi_write_fast)(spi_inst_t *spi, const uint8_t *src, size_t len);
extern void __not_in_flash_func(spi_finish)(spi_inst_t *spi);
extern void hw_read_spi(unsigned char *buff, int cnt);
extern void hw_send_spi(const unsigned char *buff, int cnt);
extern unsigned char __not_in_flash_func(hw1_swap_spi)(unsigned char data_out);

extern void lcd_spi_raise_cs(void);
extern void lcd_spi_lower_cs(void);
extern void spi_write_data(unsigned char data);
extern void spi_write_command(unsigned char data);
extern void spi_write_cd(unsigned char command, int data, ...);
extern void spi_write_data24(uint32_t data);

extern void lcd_print_string(const char *s);
extern void lcd_spi_init(void);
extern void lcd_init(void);
extern void lcd_clear(void);
extern void reset_controller(void);
extern void pin_set_bit(int pin, unsigned int offset);

// DMA チャンネルを確保して初期化する（起動時に一度だけ呼ぶ）
extern void lcd_gb_dma_init(void);
// fb の内容を LCD 用バッファに展開し、DMA 転送を開始する（非ブロッキング）
extern void lcd_gb_frame_start(const uint8_t fb[144][160]);
// DMA 転送完了を待ち、SPI / CS を後処理する
extern void lcd_gb_frame_wait(void);

#endif // VIDEO_LCD_H
