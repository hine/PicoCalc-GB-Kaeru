#pragma once
#include <stdint.h>
#include <stddef.h>

// cart RAM を path から読み込む。
// 戻り値: 0=成功, 1=ファイルなし（初回起動）, 負値=エラー
int save_sram_load(const char *path, uint8_t *ram, size_t size);

// cart RAM を path へ書き込む。/saves/ ディレクトリがなければ作成する。
// 戻り値: 0=成功, 負値=エラー
int save_sram_save(const char *path, const uint8_t *ram, size_t size);
