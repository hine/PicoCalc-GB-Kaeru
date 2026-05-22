# 作業スクラッチ

調査中の問題・仮説・試みた対策を記録する一時ファイル。
解決したら重要な気づきを PROGRESS.md に転記してセクションを削除すること。

---

## [解決済み] キー入力取りこぼし問題 (2026-05-22)

### 症状
- 「ポチ」と短く押して離すと、ほぼ取得されない
- 長押しでも認識が不安定

### 原因
- `core1_kbd_poll` が毎ループ `uint8_t joy = 0xFF` にリセットしていた
  → Core 0 が `g_joypad` を読む前に次のループで上書きされ、押下状態が消える
- `kbd_read()` は押下イベント(state==1)しか返さず、離し(state==3)を無視
  → キー状態を持続させる仕組みがなかった

### 解決策
- `kbd_read_event(int *pressed)` 追加: state==1→pressed=1、state==3→pressed=0
- `core1_kbd_poll` を persistent state 方式に変更: `joy` をループ外で保持
- press でビット OFF、release でビット ON
- A/B ボタン: z/x → `,`/`.` に変更
- フレームタイマー: `add_repeating_timer_us(-16743)` で 59.73fps 固定

### 状態
コミット 8e22815 で修正済み。実機確認済み (2026-05-22)。→ PROGRESS.md Milestone 4 に反映。
