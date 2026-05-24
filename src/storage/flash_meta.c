#include "flash_meta.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <string.h>

#define META_MAGIC_ROM   0x524F4D4Bu  // 'ROMK'
#define META_MAGIC_SRAM  0x53524D4Bu  // 'SRMK'
#define ROM_TITLE_LEN    11

typedef struct {
    uint32_t rom_magic;               // META_MAGIC_ROM if valid
    uint8_t  rom_title[ROM_TITLE_LEN];
    uint8_t  _pad0[1];
    uint32_t sram_magic;              // META_MAGIC_SRAM if valid
    uint8_t  sram_rom_title[ROM_TITLE_LEN];
    uint8_t  _pad1[1];
} meta_t;  // 32 bytes

static const meta_t *meta_xip(void) {
    return (const meta_t *)(XIP_BASE + FLASH_META_OFFSET);
}

static void meta_write(const meta_t *m) {
    static uint8_t buf[FLASH_SECTOR_SIZE];
    memset(buf, 0xFF, FLASH_SECTOR_SIZE);
    memcpy(buf, m, sizeof(meta_t));
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_META_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_META_OFFSET, buf, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
}

bool flash_meta_rom_valid(void) {
    return meta_xip()->rom_magic == META_MAGIC_ROM;
}

void flash_meta_set_rom(const uint8_t *rom_title_11) {
    meta_t m;
    memcpy(&m, meta_xip(), sizeof(m));
    m.rom_magic = META_MAGIC_ROM;
    memcpy(m.rom_title, rom_title_11, ROM_TITLE_LEN);
    meta_write(&m);
}

void flash_meta_clear_rom(void) {
    meta_t m;
    memcpy(&m, meta_xip(), sizeof(m));
    m.rom_magic = 0xFFFFFFFFu;
    memset(m.rom_title, 0xFF, ROM_TITLE_LEN);
    // ROM クリア時は SRAM も無効化（ROM が変わるため古いセーブが無意味になる）
    m.sram_magic = 0xFFFFFFFFu;
    memset(m.sram_rom_title, 0xFF, ROM_TITLE_LEN);
    meta_write(&m);
}

bool flash_meta_sram_valid(const uint8_t *rom_title_11) {
    const meta_t *m = meta_xip();
    if (m->sram_magic != META_MAGIC_SRAM) return false;
    return memcmp(m->sram_rom_title, rom_title_11, ROM_TITLE_LEN) == 0;
}

void flash_meta_set_sram(const uint8_t *rom_title_11) {
    meta_t m;
    memcpy(&m, meta_xip(), sizeof(m));
    m.sram_magic = META_MAGIC_SRAM;
    memcpy(m.sram_rom_title, rom_title_11, ROM_TITLE_LEN);
    meta_write(&m);
}

void flash_meta_clear_sram(void) {
    meta_t m;
    memcpy(&m, meta_xip(), sizeof(m));
    m.sram_magic = 0xFFFFFFFFu;
    memset(m.sram_rom_title, 0xFF, ROM_TITLE_LEN);
    meta_write(&m);
}
