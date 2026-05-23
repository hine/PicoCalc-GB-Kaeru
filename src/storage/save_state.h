#pragma once

// セーブステートをスロット番号で SD カードに保存/読み込みする。
// スロットファイルパス: 0:/saves/kaeru.sN  (N = slot 番号)
// save: 0=成功, 負値=エラー
// load: 0=成功, 1=ファイルなし, 負値=エラー
int save_state_save(int slot);
int save_state_load(int slot);
