#pragma once
#include <stdint.h>

// フラッシュ内 ROM の配置オフセット（プログラム領域と被らない 1MB 地点）
#define ROM_FLASH_OFFSET   (1u * 1024 * 1024)
#define ROM_FLASH_MAX_SIZE (512u * 1024)

// SD カード上の ROM をフラッシュへ書き込む。
// タイトルバイトが一致していればスキップ（再起動のたびに書き直さない）。
// SD はマウント済みであること。進捗は LCD に表示する。
// 戻り値: 0 = 成功 or スキップ、負値 = エラー
int rom_flash_ensure(const char *rom_path);

// タイトル一致チェックを無視して強制的に ROM を書き直す（メニュー UI からの再ロード用）。
// SD はマウント済みであること。
// 戻り値: 0 = 成功、負値 = エラー
int rom_flash_force(const char *rom_path);

// フラッシュ内 ROM データへの XIP ポインタを返す
const uint8_t *rom_flash_ptr(void);
