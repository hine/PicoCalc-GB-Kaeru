# 開発進捗

最終更新: 2026-05-22

---

## 現在のフォーカス

**Milestone 1: PicoCalc 基本I/O** — LCD Hello World・キーボード入力 実機確認済み

次のアクション：
- [ ] platform 層: SDカード ドライバ実装
- [ ] GBエミュレーションコア選定（Peanut-GB vs Walnut-CGB）

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
| 2026-05-22 | RTC は no-op スタブで対処（`compat/hardware/rtc.h`）。Milestone 6 までに再検討 | 下記「RTCに関する調査・経緯」参照 |

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
