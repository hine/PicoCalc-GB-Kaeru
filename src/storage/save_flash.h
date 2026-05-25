#pragma once
#include <stdint.h>
#include <stddef.h>

// ─── Flash セーブデータ レイアウト ────────────────────────────────────────────
// ROM 領域直後（1.5MB）からセーブデータを配置する
//
//   0x000000〜0x0FFFFF  ( 1MB)  : ファームウェア
//   0x100000〜0x17FFFF  (512KB) : ROM データ  ← ROM_FLASH_OFFSET / ROM_FLASH_MAX_SIZE
//   0x180000〜0x187FFF  ( 32KB) : SRAM セーブ
//   0x188000〜0x1C7FFF  (320KB) : セーブステート 10スロット（0-9）
//
#define SAVE_FLASH_SRAM_OFFSET    (1536u * 1024u)          // 1.5MB
#define SAVE_FLASH_SRAM_SIZE      (32u   * 1024u)          // 最大 32KB（GB cart RAM 上限）

#define SAVE_FLASH_STATE_OFFSET   (SAVE_FLASH_SRAM_OFFSET + SAVE_FLASH_SRAM_SIZE)
#define SAVE_FLASH_STATE_SLOT_SZ  (32u   * 1024u)          // 1スロット 32KB（実際は ~20KB）
#define SAVE_FLASH_STATE_N_SLOTS  10                        // index 0-9

// ─── SRAM セーブ（カートリッジ RAM）────────────────────────────────────────────
// データ形式は save_sram.c（SD 版）と同じ raw bytes なので相互コピー可能。
//
// 書き込み前に Core 1 をロックアウトまたはリセットすること。
// 読み込みは XIP 直読みなのでロックアウト不要。
//
// save: size が SAVE_FLASH_SRAM_SIZE を超える場合は何もしない
// load: 0=成功、1=未保存（領域が未書き込み）
void save_flash_sram_save(const uint8_t *ram, size_t size);
int  save_flash_sram_load(uint8_t *ram, size_t size);

// ─── セーブステート ────────────────────────────────────────────────────────────
// データ形式は save_state.c（SD 版）と同じ raw bytes なので相互コピー可能。
// slot: 0-9
//
// save: 書き込み前に Core 1 をロックアウトまたはリセットすること
// load: XIP 直読みのみ → ロックアウト不要
//       戻り値: 0=成功、1=未保存、負値=引数エラー
void save_flash_state_save(int slot);
int  save_flash_state_load(int slot);
