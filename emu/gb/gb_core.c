#include "gb_core.h"
#include "storage/rom_flash.h"
#include <stdlib.h>
#include <string.h>

#define ENABLE_LCD 1
#include "peanut_gb.h"

typedef struct {
    const uint8_t *rom;
    uint8_t *cart_ram;
} rom_ctx_t;

static struct gb_s gb;
static rom_ctx_t  ctx;

uint8_t gb_fb[GB_SCREEN_H][GB_SCREEN_W];

static uint8_t rom_read_cb(struct gb_s *g, const uint_fast32_t addr)
{
    return ((const rom_ctx_t *)g->direct.priv)->rom[addr];
}

static uint8_t cart_ram_read_cb(struct gb_s *g, const uint_fast32_t addr)
{
    const rom_ctx_t *r = g->direct.priv;
    return r->cart_ram ? r->cart_ram[addr] : 0xFF;
}

static void cart_ram_write_cb(struct gb_s *g, const uint_fast32_t addr,
                               const uint8_t val)
{
    rom_ctx_t *r = g->direct.priv;
    if (r->cart_ram) r->cart_ram[addr] = val;
}

static void error_cb(struct gb_s *g, const enum gb_error_e err,
                     const uint16_t val)
{
    (void)g; (void)err; (void)val;
}

static void lcd_line_cb(struct gb_s *g, const uint8_t *pixels,
                        const uint_fast8_t line)
{
    (void)g;
    if (line < LCD_HEIGHT)
        memcpy(gb_fb[line], pixels, LCD_WIDTH);
}

int gb_core_init(void)
{
    ctx.rom      = rom_flash_ptr();
    ctx.cart_ram = NULL;

    enum gb_init_error_e err = gb_init(&gb,
        rom_read_cb, cart_ram_read_cb, cart_ram_write_cb,
        error_cb, &ctx);
    if (err != GB_INIT_NO_ERROR) return -(int)err - 10;

    gb_init_lcd(&gb, lcd_line_cb);

    size_t save_size = 0;
    gb_get_save_size_s(&gb, &save_size);
    if (save_size > 0) {
        ctx.cart_ram = malloc(save_size);
        if (!ctx.cart_ram) return -3;
        memset(ctx.cart_ram, 0xFF, save_size);
    }

    return 0;
}

void gb_core_run_frame(void)
{
    gb_run_frame(&gb);
}

void gb_core_set_joypad(uint8_t joypad)
{
    gb.direct.joypad = joypad;
}
