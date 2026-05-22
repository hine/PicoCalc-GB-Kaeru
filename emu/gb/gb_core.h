#pragma once
#include <stdint.h>

#define GB_SCREEN_W 160
#define GB_SCREEN_H 144

// DMG パレットインデックス (0-3) のフレームバッファ。
// gb_core_run_frame() 呼び出し後に更新される。
extern uint8_t gb_fb[GB_SCREEN_H][GB_SCREEN_W];

// フラッシュ上の ROM を使ってエミュレータを初期化する。
// rom_flash_ensure() を先に呼ぶこと。
// 戻り値: 0 = 成功、負値 = エラー
int  gb_core_init(void);

// 1 フレーム分エミュレートして gb_fb を更新する。
void gb_core_run_frame(void);

// ジョイパッドの状態をセットする。
// JOYPAD_* 定数のビット OR で指定。0 = 押下、1 = 未押下。
// 初期値はすべて未押下 (0xFF)。
void gb_core_set_joypad(uint8_t joypad);

// lcd_line_cb が書き込む先のフレームバッファを切り替える（ダブルバッファ用）。
void gb_core_set_fb(uint8_t (*fb)[GB_SCREEN_W]);
