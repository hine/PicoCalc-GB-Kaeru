# 作業スクラッチ

調査中の問題・仮説・試みた対策を記録する一時ファイル。
解決したら重要な気づきを PROGRESS.md に転記してセクションを削除すること。

---

## [解決済み] USB なし起動でキーボード認識しない問題 (2026-05-23)

### 症状
- USB を刺さずに起動するとキーボードが認識されない
- USB を刺してから起動すると正常動作

### 仮説
USB 接続時は USB 初期化処理の分だけ起動に時間がかかり、その間にキーボードコントローラ（I2C 0x1F）が起動できている。
USB なし起動では Pico が先に起動し、キーボードコントローラの準備ができる前に I2C アクセスが走って stuck bus になっている可能性。

### 経緯
- 以前 `kbd_wait_power()` で KB コントローラ応答待ちをしていたが、Core 0 から呼ぶと SD マウントが詰まるため廃止
- 代わりに Core 1 の `sleep_ms(500)` で代替していたが、500ms では足りないケースがある模様

### 対策方針
`kbd_wait_power()` 相当のロジック（I2C 再初期化 + ping ループ）を Core 1 の起動時に移植する。
Core 1 は GB エミュレータ（Core 0）とは独立して動くため、どれだけ待っても画面描画には影響しない。

```
core1_kbd_poll():
  loop:
    kbd_init()          ← I2C 再初期化（stuck bus リカバリ）
    i2c_write ping      ← 応答チェック
    if ok: break
    sleep_ms(200)
  → 通常ポーリングへ
```

### 解決（仮）→ 実機確認で未解決

`kbd_wait_ready()` を Core 1 から呼ぶ形で実装（コミット 7ea21be）。
実機で再確認したがまだ駄目。

### 追加調査 (2026-05-23)

**問題の本質**: I2C stuck bus（SDA が Low に張り付く）

- PicoCalc 起動時、キーボードコントローラが起動中に I2C SDA を Low に引き下げることがある
- `i2c_init()` は I2C ペリフェラルをリセットするが、スレーブが SDA を Low に保持している状態を解除しない
- → `i2c_write_timeout_us` が毎回タイムアウトし、`kbd_wait_ready()` が永久ループになっている可能性
- USB 接続時は USB 初期化の遅延（数百ms）の間にコントローラが起動し SDA が解放される

**正しい対処**: I2C バスリカバリ（Bus Recovery, I2C spec §3.1.16）
- SDA が Low に張り付いている場合、SCL を GPIO として最大 9 パルス出力して解放する
- その後 STOP コンディションを発行してバスを確実にリセット
- その後 i2c_init() で正規 I2C として再初期化

実装方針: `kbd_wait_ready()` の各リトライ前に `i2c_bus_recover()` を呼ぶ

### 追加調査 — リグレッション (2026-05-23)

`i2c_bus_recover()` 追加後、USB 接続起動でも認識しなくなった。

**原因**: `gpio_init(SDA)` が内部で出力レジスタを 0 に設定する。
その後 `gpio_set_dir(SDA, GPIO_OUT)` を呼ぶと **SDA が即座に LOW になる**（SCL は HIGH のまま）。
→ SDA が stuck していない通常ケースでも **意図しない START コンディションを発行**してしまう。
→ その後 SDA HIGH にすると STOP になり、KB コントローラが混乱する可能性。

`if (gpio_get(I2C_KBD_SDA)) return;` を追加して USB boot のリグレッションは修正済み。

### 追加調査 — バスリカバリでも standalone が直らない (2026-05-23)

kbd_wait_ready() + バスリカバリを実装しても standalone NG のまま。

**新仮説**: バスリカバリ自体が問題を悪化させている可能性。
- KB コントローラがまだ起動中（SDA を LOW にしている）の状態で SCL を 9 パルス叩くと、
  コントローラ側の I2C ステートマシンが混乱して「起動後も応答しない」状態になるかもしれない。
- また、9 パルスで SDA が解放されない場合でも START+STOP を発行しているため、
  bus contention（push-pull HIGH vs open-drain LOW）が発生している可能性。

**方針転換**: バスリカバリを取り除き、「ひたすら待つ」シンプル戦略に戻す。
- standalone 起動時、KB コントローラが完全に起動するまでには数秒かかるかもしれない
- `kbd_init()` を Core 0 の main() から除去（I2C1 を premature に初期化しない）
- Core 1 で 2 秒の初期待機 + 500ms 間隔のリトライループ

→ 依然として standalone NG。

### 根本原因の発見 (2026-05-23)

HardwareSpec.md + git diff の照合で判明:

**lcd_init() は 240ms 以上かかる**（Sleep Out 後 120ms × 2 のウエイトが仕様に含まれる）。

従来のコード:
```
main() → lcd_init() → kbd_init() → ...
          ^^^^^^^^^^^^
          この 240ms の間、GP6/7 は pull-up なし（浮動）
```

standalone 起動では Pico と STM32 キーボードコントローラが**同時**に起動する。
STM32 は起動直後に自分の I2C ペリフェラルを初期化する。
その瞬間 SCL/SDA が浮動状態（pull-up なし）だと、
STM32 が「I2C バスが存在しない」と判定して I2C インターフェースを正常に起動できない。

USB 起動では KB コントローラは PicoCalc 電源ボタンが押されるまで無電源のため、
電源投入時点で既に kbd_init() が完了しており SCL/SDA=HIGH が確立済み。
→ USB では問題が出ない。

**修正方針**: `kbd_init()` を `stdio_init_all()` の直後、`lcd_init()` の**前**に移動する。
これにより起動直後から SCL/SDA=HIGH（idle I2C バス）が確立され、
STM32 が正常に I2C インターフェースを初期化できる。

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
