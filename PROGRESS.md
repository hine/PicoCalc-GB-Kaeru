# 開発進捗

最終更新: 2026-05-22

---

## 現在のフォーカス

**Milestone 1: PicoCalc 基本I/O** — ハードウェア仕様調査完了、platform 層実装準備中

次のアクション：
- [ ] GBエミュレーションコア選定（Peanut-GB vs Walnut-CGB）
- [ ] platform 層: LCD ドライバ実装
- [ ] platform 層: キーボード ドライバ実装
- [ ] platform 層: SDカード ドライバ実装

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
- [ ] LCD 初期化・Hello World 表示
- [ ] キーボード入力取得
- [ ] SDカード読み込み確認
- [ ] スピーカー簡易出力

### Milestone 2: GBコア組み込み ⬜ 未着手

- [ ] GBエミュレーションコア選定
- [ ] Peanut-GB / Walnut-CGB 評価
- [ ] emu 層構成決定
- [ ] ROM 読み込み
- [ ] CPU 実行
- [ ] 映像バッファ取得

### Milestone 3: 画面表示 ⬜ 未着手

- [ ] 160×144 画面を LCD へ表示
- [ ] フレーム更新ループ
- [ ] 簡易 FPS 計測

### Milestone 4: 入力対応 ⬜ 未着手

- [ ] 十字キー
- [ ] A / B
- [ ] Start / Select
- [ ] Menu（Esc）

### Milestone 5: 対象ROM起動 ⬜ 未着手

- [ ] 「カエルのために鐘は鳴る」タイトル表示
- [ ] ゲーム開始
- [ ] 操作可能状態

### Milestone 6: セーブ対応 ⬜ 未着手

- [ ] SRAM 保存 / 読み込み
- [ ] SD への保存

### Milestone 7: 音声対応 ⬜ 未着手

- [ ] GB APU 出力
- [ ] PicoCalc スピーカー再生
- [ ] 音量調整

### Milestone 8: GBC 対応 ⬜ 未着手

- [ ] GBC ROM 起動
- [ ] カラー表示
- [ ] 必要な MBC 対応

### Milestone 9: 携帯機化 ⬜ 未着手

- [ ] セーブステート
- [ ] スリープ / 自動復帰
- [ ] メニュー UI

---

## 決定事項ログ

| 日付 | 決定内容 | 理由 |
|---|---|---|
| 2026-05-22 | 実行ファイル名を `picocalc_gb_kaeru` とする | ターゲットROM名と一致させ識別しやすくするため |
| 2026-05-22 | Peanut-GB が有力候補 | PicoCalc + RP2350 向け既存実装の大多数が採用、軽量・移植性高い |
| 2026-05-22 | LCD は SPI1 (GP10-15)、10kHz I2C キーボード (GP6/7, 0x1F)、SD は SPI0 (GP16-19) | PicoCalc ハードウェア仕様調査結果より |

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
| 音声方式 | 初期は PWM、後で I2S（GP26/27）への移行を検討 |
