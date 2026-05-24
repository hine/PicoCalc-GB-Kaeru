# 開発進捗

最終更新: 2026-05-24（音声改善・キーバインド更新）

---

## 現在のフォーカス

**Milestone 9: 携帯機化**

次のアクション：
- [ ] メニュー UI（Enter/Esc をグローバルメニュー操作に使用予定）
- [ ] バッテリー残量表示（キーボードコントローラ I2C レジスタ 0x0B）
- [ ] 音量調整（将来のメニュー UI で設定）

---

## マイルストーン進捗

### Milestone 0: 開発環境構築 ✅ 完了

- [x] Pico SDK 導入（`~/dev/pico-sdk`）
- [x] CMake / Ninja ビルド確認
- [x] UF2 生成確認（`build/picocalc_gb_kaeru.uf2`）
- [x] PicoCalc への書き込み確認
- [x] pyenv virtualenv 構築（`picocalc_gb_kaeru` / Python 3.13.13）
- [x] `compile_commands.json` 生成設定

### Milestone 1: PicoCalc 基本I/O 🔄 進行中

- [x] PicoCalc LCD 仕様調査（→ `HardwareSpec.md`）
- [x] PicoCalc キーボード 仕様調査（→ `HardwareSpec.md`）
- [x] PicoCalc SDカード 仕様調査（→ `HardwareSpec.md`）
- [x] 既存 GB エミュレータ実装調査（→ `HardwareSpec.md`）
- [x] LCD 初期化・Hello World 表示（緑文字で実機確認済み 2026-05-22）
- [x] キーボード入力取得（実機確認済み 2026-05-22）
- [x] SDカード読み込み確認（実機確認済み 2026-05-22）
- [ ] スピーカー簡易出力

### Milestone 2: GBコア組み込み ✅ 完了

- [x] GBエミュレーションコア選定（Peanut-GB 採用）
- [x] emu 層構成決定（`emu/gb/gb_core.c`）
- [x] ROM 読み込み（初回起動時に Flash へ書き込み、XIP で直読み）
- [x] CPU 実行（実機 ~60fps 確認済み 2026-05-22）
- [x] 映像バッファ取得（`gb_fb[144][160]` に lcd_draw_line コールバックで蓄積）

### Milestone 3: 画面表示 ✅ 完了

- [x] 160×144 画面を LCD へ表示（1x, 中央配置 x=80, y=88 実機確認済み 2026-05-22）
- [x] フレーム更新ループ（DMA ダブルバッファで ~60fps 達成 2026-05-22）
- [ ] 簡易 FPS 計測

### Milestone 4: 入力対応 ✅ 完了

- [x] 十字キー（KEY_UP/DOWN/LEFT/RIGHT）
- [x] 十字キー（KEY_UP/DOWN/LEFT/RIGHT + WASD 追加 2026-05-24）
- [x] A / B（`,`/`[`=A、`.`/`]`=B、2026-05-24 更新）
- [x] Start / Select（BS=Start、Del=Select、2026-05-24 更新）
- [x] press/release 状態管理（「ポチ」押しの取りこぼし修正 実機確認済み 2026-05-22）
- [ ] Menu（Enter/Esc）→ Milestone 9 で対応

### Milestone 5: 対象ROM起動 ✅ 完了

- [x] 「カエルのために鐘は鳴る」タイトル表示（実機確認済み 2026-05-23）
- [x] ゲーム開始（実機確認済み 2026-05-23）
- [x] 操作可能状態（USB・standalone 両方確認済み 2026-05-23）

### Milestone 6: セーブ対応 ✅ 完了

- [x] SRAM 保存 / 読み込み
- [x] SD への保存（30 秒間隔自動セーブ、`/saves/kaeru.sav`、実機確認済み 2026-05-23）

### Milestone 7: 音声対応 🔄 進行中

- [x] GB APU 出力（minigb_apu 統合、実機確認済み 2026-05-23）
- [x] PicoCalc スピーカー再生（PWM 12bit + DMA IRQ ダブルバッファ、実機確認済み 2026-05-24）
- [x] 音声 Core 1 移行（SPSC リングバッファ経由、LCD 描画との干渉解消、実機確認済み 2026-05-24）
- [x] DMA IRQ 駆動音声補充（ポーリング廃止、レイテンシ ~33ms）
- [x] RP2350 クロック不一致修正（TIMER_WRAP 3814→4576、音程ズレ +2.4 半音を解消）
- [ ] 音量調整（将来のメニュー UI で設定）

### Milestone 8: GBC 対応 ⬜ 未着手

- [ ] GBC ROM 起動
- [ ] カラー表示
- [ ] 必要な MBC 対応

### Milestone 9: 携帯機化 🔄 進行中

- [x] ステータスバー UI（上部 16px・下部 16px ストリップ、実装済み 2026-05-23）
- [x] セーブステート（F2=保存/F3=スロット切替/F4=ロード、実装済み 2026-05-23）
- [x] スリープ dormant（F1=スリープ/F1=復帰、実装済み 2026-05-23。go_dormant() 安定化 2026-05-24）
- [x] LovyanGFX 移行（RGB565 差分描画、等倍テキスト UI、実機確認済み 2026-05-24）
- [x] LCD 描画 Core 1 移行（ダブルフレームバッファ + mutex、Core 0 を GB 実行専用に、実機確認済み 2026-05-24）
- [x] 音声 Core 1 移行・DMA IRQ 駆動・PWM 12bit・音程修正（実機確認済み 2026-05-24）
- [x] キーバインド更新（WASD 追加・`[`/`]` AB ボタン・BS=Start/Del=Select、2026-05-24）
- [ ] メニュー UI（Enter/Esc 使用予定）
- [ ] バッテリー残量表示
- [ ] 音量調整

#### UI レイアウト設計メモ（2026-05-23 検討済み）

- LCD: 320×320、GB 画面: 2x スケール = 320×288（y=16〜303）
- **上部ストリップ** y=0〜15 (16px): ステータスマーク専用。SD アクセスランプ等の小アイコン。
  - フォント 8×12 が収まる（2px 上下パディング）
  - 現状は黒（ゲーム中は何も表示しない）、将来的に右端に 8×8 アイコンを置く予定
- **下部ストリップ** y=304〜319 (16px): キーヒント表示候補。
  - 8×12 フォントで 1 行表示可能（`","=A  "."=B  BS=Sel  ENT=Sta` など 40 文字幅の余裕あり）
  - 暗めの背景（RGB 24,24,24）にグレー文字で描画予定
- 場合によって上 8px / 下 24px に変更する可能性あり（上はマークのみのため）
- 実装方針: `lcd_status_draw_hints()` を起動時に一度呼ぶ、`lcd_status_sd_icon(bool)` を autosave 前後に呼ぶ

---

## 決定事項ログ

| 日付 | 決定内容 | 理由 |
|---|---|---|
| 2026-05-22 | 実行ファイル名を `picocalc_gb_kaeru` とする | ターゲットROM名と一致させ識別しやすくするため |
| 2026-05-22 | Peanut-GB が有力候補 | PicoCalc + RP2350 向け既存実装の大多数が採用、軽量・移植性高い |
| 2026-05-22 | LCD は SPI1 (GP10-15)、10kHz I2C キーボード (GP6/7, 0x1F)、SD は SPI0 (GP16-19) | PicoCalc ハードウェア仕様調査結果より |
| 2026-05-22 | RTC は no-op スタブで対処（`compat/hardware/rtc.h`）。Milestone 6 までに再検討 | 下記「RTCに関する調査・経緯」参照 |
| 2026-05-22 | USB stdio を無効化（`pico_enable_stdio_usb 0`） | TinyUSB が SPI0/DMA と競合し SD マウント失敗するため |
| 2026-05-22 | `kbd_wait_power()` を廃止し `sd_mount()` リトライループで代替 | PicoCalc 同時起動時に KB コントローラの I2C stuck bus が発生し永久待機になるため。SD マウント自体を直接リトライする方が堅牢 |
| 2026-05-23 | `kbd_init()` を `lcd_init()` より前（main 冒頭）に移動 | standalone 起動時、lcd_init の 240ms+ ウエイト中に STM32 が I2C 初期化する。この時点で SCL/SDA が浮動だと STM32 の I2C が正常起動しない。詳細は HardwareSpec §3.5 |
| 2026-05-23 | キーボードポーリングを Core 1 の `kbd_wait_ready()` リトライループに移行 | kbd_init 後も STM32 の I2C 起動完了まで数秒かかる。Core 1 でリトライすることで Core 0（GB エミュレータ）をブロックしない |
| 2026-05-22 | ROM を Flash XIP で提供（`src/storage/rom_flash.c`） | SD バンク読み込みが 1 フレームに 8 回発生し 7.5fps 止まりだったため。Flash XIP に切替後 ~60fps 達成 |
| 2026-05-22 | LCD 表示を 1x スケール + DMA ダブルバッファに変更 | 2x ブロッキング転送(276KB@25MHz)が 88ms/frame で 6-10fps だったため。1x(69KB@37.5MHz) + DMA で ~60fps 達成 |
| 2026-05-22 | GBエミュレーションコアは Peanut-GB（ヘッダオンリー）を採用 | 軽量・移植性高い・RP2350 実績あり |
| 2026-05-23 | LovyanGFX 導入は Milestone 9 まで先送り | ILI9488 対応・CMake 統合（add_subdirectory）とも問題ないが、現在の DMA delta 描画は既に ~60fps で十分。音声対応への寄与もなし。メニュー UI 実装時に改めて評価する |
| 2026-05-24 | LovyanGFX 移行完了。RGB565 + swap=true を採用 | RGB888 よりバッファ 1/3 小さく代入も軽い。ILI9488 は常に 18bit 送信のため SPI 帯域は同じ。色精度はパレット数値で後調整可能 |
| 2026-05-24 | lgfx_config: invert=true 必須、rgb_order=true、setRotation(6) | ILI9488 パネルは自然反転あり → INVON で補正。setRotation(6)=MX=0/MY=0 が正常ポートレート（0=左右反転、4=180°）|
| 2026-05-24 | UI テキストを textSize=1（等倍）に統一 | 起動画面・ステータスバーはゲームループ外で速度影響ゼロ。Font0 6×8px、16px ストリップ内 Y+4 で縦中央揃え |
| 2026-05-24 | LovyanGFX SD タイミング問題を sleep_ms(1000) で修正 | 旧 lcd_init() は RST 後 ~240ms 待機し SD 起動猶予を確保していた。LovyanGFX は ~20ms で完了するため SD がマウント失敗。明示的 sleep で解決 |
| 2026-05-24 | go_dormant(): __wfi() 方式に変更、IRQ 停止必須化 | tight_loop_contents() では WFI が発行されず dormant に入れなかった。PRIMASK=1 + ペンディング IRQ でも WFI がすぐ返るため、frame_timer と audio DMA を先に停止してから呼ぶこと |
| 2026-05-24 | I2S 化を断念・PWM 継続 | PicoCalc には I2S DAC が搭載されていない（GP26/27 はアンプ直結 PWM 専用）。ハードウェア的に I2S 不可 |
| 2026-05-24 | 音声を Core 1 へ移行（SPSC リングバッファ + DMA IRQ 駆動） | Core 0（GB 実行）が LCD SPI 転送でブロックされると音声が途切れていた。Core 1 で DMA IRQ が直接リングバッファを消費することで両者を分離 |
| 2026-05-24 | LCD 描画も Core 1 へ移行（ダブルフレームバッファ + mutex） | 音声と LCD を同一コアで処理し Core 0 を GB 実行専用に。g_lcd_busy フラグでフレームドロップ方式を採用（Core 0 はブロックしない） |
| 2026-05-24 | DMA_BUF_SAMPLES を 2048→548 に戻し DMA IRQ 駆動補充に変更 | 2048 サンプル（~125ms）では音声レイテンシが大きく音質劣化。IRQ 駆動にすることでポーリング不要・低レイテンシ（~33ms）を両立 |
| 2026-05-24 | TIMER_WRAP を 3814→4576 に修正（RP2350 150MHz 対応） | RP2350 デフォルトクロックは 150MHz だが 125MHz 前提で計算していた。誤: 150MHz/3815=39319Hz（+2.4 半音高）→ 正: 150MHz/4577=32773Hz（誤差 0.016%）|
| 2026-05-24 | PWM 解像度を 10bit → 12bit に拡張（AUDIO_WRAP 1023→4095） | DMA 転送量は変わらず無コスト。量子化ノイズ下限 −60dB→−72dB。キャリア周波数 146kHz→36.6kHz（可聴域外を維持）|
| 2026-05-24 | キーバインド更新 | WASD を十字キーに追加。`[`/`]` を A/B ボタンに追加（`,`/`.` は維持）。Enter をスタートから解除し BS=スタート・Del=セレクトに変更。Enter/Esc はメニュー UI 用に確保 |

---

## 調査済み情報

| トピック | 場所 |
|---|---|
| PicoCalc ハードウェア仕様（LCD・KB・SD・Audio）| `HardwareSpec.md` |
| 開発環境構築手順 | `DevelopmentEnvironment.md` |
| プロジェクト全体仕様・方針 | `Spec.md` |

---

## 保留・未解決事項

| 項目 | 内容 |
|---|---|
| LCDコントローラ世代 | 所有する PicoCalc が ILI9488 か ST7365P かは実機確認が必要 |
| GBCエミュコア | GBC対応は Milestone 8 まで保留。Walnut-CGB の評価は Milestone 2 で実施 |
| PSRAM 活用 | RP2350 + PicoCalc に PSRAM がある場合、フレームバッファ拡張に活用可能 |
| 音声方式 | PWM 12bit（GP26/27、DMA IRQ 駆動、Core 1 処理）で安定動作中。I2S DAC 非搭載のため I2S 化は不可。残課題は音量調整のみ |
| RTC / タイムスタンプ | 現状 no-op スタブ。セーブステート複数管理時に要再検討（下記参照） |

---

## RTCに関する調査・経緯（2026-05-22）

### 問題の発生

SDカード向けFatFSライブラリ `no-OS-FatFS-SD-SPI-RPi-Pico`（carlk3）を導入した際、
`rtc.c` が `hardware/rtc.h` をインクルードしているためにビルドエラーが発生。

### 原因

RP2040 にはハードウェア RTC が搭載されていたが、**RP2350 では廃止**された。
代替は `pico_aon_timer`（POWMAN timer）だが、FatFS ライブラリはまだ対応していない。
同様の問題が他プロジェクトでも報告されており未解決のまま：
→ https://github.com/rppicomidi/midi2piousbhub/issues/10

### PicoCalc 本体への外部 RTC 搭載状況

公式リポジトリ・フォーラム・ピン割り当て表を調査した結果、
**PicoCalc には外部 RTC チップ（DS3231 等）は搭載されていない**。
I2C1（GP6/7）はキーボードコントローラ（0x1F）に占有されており、
RTC 用の I2C アドレスやピンの記載は存在しない。

### 現在の対処（暫定）

`compat/hardware/rtc.h` にスタブを実装：
- `datetime_t` 構造体を定義
- `rtc_init()` / `rtc_get_datetime()` / `rtc_set_datetime()` を no-op 化
- FatFS の `get_fattime()` は常に 0 を返す → ファイルの日時は **1980-01-01** 固定

あわせて CMakeLists.txt で空の `hardware_rtc` INTERFACEターゲットを定義し、
リンクエラーを回避している。

### セーブファイル管理への影響と今後の方針

タイムスタンプが固定のため、**複数のセーブステートファイルを日時で区別することができない**。
Milestone 6（セーブ対応）・Milestone 9（セーブステート）の実装時に対策が必要。

候補となる対処方針：

| 方針 | 概要 | 難易度 |
|---|---|---|
| A. ファイル名に連番を付与 | `kaeru.slot0.state`, `slot1.state` など番号管理 | 低 |
| B. 起動回数カウンタを Flash に保存 | リセット後も単調増加する番号でファイル名生成 | 中 |
| C. `pico_aon_timer` で RTC 代替実装 | RP2350 の AON タイマーで時刻管理。電池バックアップなし | 高 |
| D. 外部 RTC モジュールを追加 | DS3231 等を I2C0（空きピン）に接続 | 高（ハードウェア改造） |

**当面は方針 A（連番スロット）で実装し、Milestone 6 で正式決定する。**
