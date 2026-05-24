# Spec.md

## 1. プロジェクト概要

本プロジェクトは、RP2350搭載ボードをPicoCalcに装着し、Game Boy / Game Boy Color互換機能を実装する試作プロジェクトである。

初期ターゲットは、所有済みROMである「カエルのために鐘は鳴る」をPicoCalc上で動作させることとする。

本プロジェクトは以下の4系統開発のうち、B-2に該当する。

| ID | 方針 |
|---|---|
| A-1 | ESP32-S3系を用いたThe Castle専用MSX1端末 |
| A-2 | ESP32-S3系を用いたGame Boy固定ROM端末 |
| B-1 | RP2350 + PicoCalcを用いたMSX1互換機 |
| B-2 | RP2350 + PicoCalcを用いたGB/GBC互換機 |

本仕様書では、B-2の初期実装方針を定義する。

---

## 2. 目的

### 2.1 最初の目的

PicoCalc上で、Game Boy ROMを起動し、画面表示・キー入力・基本動作を確認する。

最初の対象ROMは以下とする。

- カエルのために鐘は鳴る
- DMG Game Boy用ROM
- ROMはユーザーが所有し、吸い出し済み

### 2.2 中期目的

以下を実装する。

- GB互換動作
- GBC互換動作
- セーブRAM対応
- セーブステート対応
- PicoCalcキーボードによる操作
- SDカードからのROM読み込み
- LCDへの適切なスケーリング表示
- 音声出力
- 省電力・スリープ復帰

### 2.3 長期目的

PicoCalcを「ポケットGB/GBC互換機」として利用できる状態にする。

将来的には、同じエミュレーションコアや周辺抽象化をA-2のESP32-S3端末へ移植できる構造を目指す。

---

## 3. 対象ハードウェア

### 3.1 本体

- ClockworkPi PicoCalc

### 3.2 SoCボード

PicoCalc内部のRaspberry Pi Pico互換ピンに装着するRP2350A搭載ボードを使用する。

想定仕様：

| 項目 | 内容 |
|---|---|
| SoC | Raspberry Pi RP2350A |
| CPU | Dual-core Arm Cortex-M33 |
| 追加CPU | Dual-core Hazard3 RISC-V |
| 最大クロック | 150MHz |
| SRAM | 520KB |
| Flash | 16MB |
| USB | USB Type-C / USB 1.1 Device・Host |
| GPIO | 26本 |
| PIO | 12ステートマシン |
| ADC | 4ch 12-bit |
| PWM | 16ch |
| 電源 | LiPo充放電ヘッダーあり |
| DC-DC | MP28164、最大2A負荷対応 |

### 3.3 PicoCalc側機能

使用予定のPicoCalc側機能：

- LCD
- キーボード
- microSD
- スピーカー
- バッテリー
- 電源管理
- 外部ケース・筐体

---

## 4. 開発環境

### 4.1 基本方針

Codex / Claude CodeなどのCLI支援を受けやすいよう、コマンドラインで完結する開発環境を採用する。

### 4.2 推奨環境

第一候補：

- Raspberry Pi Pico SDK
- CMake
- Ninja
- picotool

PlatformIOはESP32-S3系では有力だが、RP2350 + PicoCalcではPico SDKを第一候補とする。

### 4.3 想定ビルドコマンド

```
cmake -S . -B build -G Ninja -DPICO_BOARD=pico2 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

---

## 5. ソフトウェア構成

### 5.1 ディレクトリ構成（現状）

```
picocalc-gb-kaeru/
  Spec.md / PROGRESS.md / HardwareSpec.md / README.md
  CMakeLists.txt
  hw_config.c              # FatFs_SPI ハードウェア設定
  compat/hardware/rtc.h   # RP2350 で廃止された hardware_rtc のスタブ
  src/
    main.c
    video/
      video_lcd.h          # LCD API（lcd_init / lcd_gb_frame_delta 等）
      video_lgfx.cpp       # LovyanGFX 実装（ILI9488、差分描画、ステータスバー）
      lgfx_config.hpp      # LovyanGFX パネル設定
    input/
      input_keyboard.c/h   # STM32 キーボードコントローラ I2C ポーリング
    audio/
      audio_pwm.c/h        # PWM 12bit + DMA IRQ ダブルバッファ（Core 1）
    storage/
      storage_sd.c/h       # SD マウント/アンマウント
      rom_flash.c/h        # ROM を Flash XIP に書き込み・検証
      save_flash.c/h       # SRAM セーブ・セーブステートを Flash に保存
      flash_meta.c/h       # Flash メタデータ（ROM 有効フラグ・SRAM 検証）
      save_sram.c/h        # SD バックアップ用 SRAM 保存（将来の UI から呼び出し）
      save_state.c/h       # SD バックアップ用ステート保存（将来の UI から呼び出し）
  emu/
    gb/
      gb_core.c/h          # Peanut-GB ラッパー（ROM XIP・APU・セーブステート統合）
  lib/
    peanut-gb/             # GB/GBC エミュレーションコア（ヘッダオンリー）
    LovyanGFX/             # LCD ドライバライブラリ
    no-OS-FatFS-SD-SPI-RPi-Pico/  # FatFS + SPI SD ドライバ
```

### 5.2 Git管理方針

ROMファイル、BIOS、セーブデータはGit管理しない。

`.gitignore` に以下を含める。

```
build/
*.uf2
*.elf
*.bin
roms/*.gb
roms/*.gbc
saves/*
*.sav
*.state
```

---

## 6. エミュレーション対象

### 6.1 初期対象

- Game Boy / DMG
- 固定ROMまたはSDカード上の単一ROM

### 6.2 次段階

- Game Boy Color / GBC
- MBC対応
- SRAMセーブ対応

### 6.3 対象外

初期段階では以下は対象外とする。

- Game Boy Advance
- 通信ケーブル
- 赤外線通信
- RTC付き特殊カートリッジ
- 完全なタイミング互換性
- 全市販ROMの完全動作

---

## 7. 表示仕様

### 7.1 Game Boy表示

Game Boyの画面解像度は160×144である。

PicoCalcのLCD解像度に応じて、以下のいずれかの表示方式を採用する。

- 等倍表示
- 整数倍表示
- アスペクト比維持拡大
- 周囲余白にステータス表示

### 7.2 初期実装

初期実装では、処理負荷を抑えるため以下を優先する。

- フルスクリーン拡大より安定動作を優先
- まずはモノクロ/DMG表示
- フレームバッファは必要最小限
- LCD転送はDMA活用を検討

### 7.3 画面周辺UI

将来的に以下を表示する。

- ROM名
- FPS
- バッテリー状態
- セーブ状態
- 時計
- 操作ヘルプ

---

## 8. 入力仕様

### 8.1 現在のキーマッピング（実装済み）

| GB入力 | 主キー | 副キー |
|---|---|---|
| Up | カーソル↑ | W |
| Down | カーソル↓ | S |
| Left | カーソル← | A |
| Right | カーソル→ | D |
| A ボタン | `,` | `[` |
| B ボタン | `.` | `]` |
| Start | BS（Backspace） | — |
| Select | Del | — |

### 8.2 システムキー（ゲームに渡さない）

| キー | 機能 |
|---|---|
| F1 | スリープ（dormant）/ 復帰 |
| F2 | セーブステート保存（現在スロット） |
| F3 | セーブスロット切り替え（0〜9 サイクル） |
| F4 | セーブステートロード（現在スロット） |
| Enter | グローバルメニュー用（将来実装） |
| Esc | グローバルメニュー用（将来実装） |

### 8.3 方針

- WASD は十字キー追加マッピング（カーソルキーと同時使用可）
- Enter / Esc はゲームキーに割り当てず、将来のメニュー UI 専用に確保
- キーマッピングは `src/main.c` の `key_to_joypad_bit()` で管理

---

## 9. 音声仕様

### 9.1 実装済み構成

| 項目 | 内容 |
|---|---|
| 出力方式 | PWM（GP26/GP27、PicoCalc アンプ直結） |
| 解像度 | 12bit（AUDIO_WRAP=4095） |
| サンプリング周波数 | 32768 Hz（TIMER_WRAP=4576 @ 150MHz） |
| バッファ | DMA ダブルバッファ（548 サンプル/バッファ、~33ms レイテンシ） |
| 駆動方式 | DMA IRQ 駆動（Core 1 で処理、ポーリング不要） |
| APU コア | minigb_apu（Peanut-GB 同梱、S16 ステレオ） |
| データ経路 | Core 0: APU → SPSC リングバッファ → Core 1: DMA → PWM |

### 9.2 設計上の注意

- I2S DAC 非搭載のため I2S 化は不可（GP26/27 はアンプ直結 PWM 専用）
- RP2350 デフォルトクロックは 150MHz（125MHz 前提の TIMER_WRAP は音程ズレを生じる）
- PWM キャリア周波数 = 150MHz / 4096 ≈ 36.6kHz（可聴域外）

### 9.3 残課題

- 音量調整（将来のメニュー UI で設定）
- ミュート機能

---

## 10. ストレージ仕様

### 10.1 基本方針

SD カードはカード破損リスクがあるため、**ゲームデータの永続保存には Flash を使用する**。SD は初回 ROM ロード時と将来のバックアップ UI のみで使用する。

### 10.2 Flash レイアウト

```
アドレス       サイズ    用途
0x000000      1 MB      ファームウェア（Pico SDK ビルド成果物）
0x100000    512 KB      ROM データ（XIP 直読み）
0x180000     32 KB      SRAM セーブ（cart RAM の raw dump）
0x188000    352 KB      セーブステート × 11 スロット（0〜9: 通常、10: スリープ用）
0x1E0000      4 KB      Flash メタデータ（ROM 有効フラグ・SRAM 有効フラグ）
─────────────────────────────────────────
計           1.879 MB   使用済み（4 MB Flash に対して余裕あり）
```

### 10.3 ROM 管理

- 初回起動時に SD カードの `/roms/kaeru.gb` を Flash に書き込む
- 2 回目以降は Flash メタデータの ROM 有効フラグを確認し、SD なしで起動可能
- SD が挿入されている場合は ROM タイトル照合を行い、差分があれば再フラッシュ
- `flash_meta_clear_rom()` を呼ぶと次回起動時に SD からの再ロードを強制できる

### 10.4 SRAM セーブ

- ゲームが cart RAM に書き込んだ後、~1 秒（60 フレーム）のデバウンスを経て自動保存
- Flash メタデータにマジック + ROM タイトル（11 バイト）を記録し、ロード時に検証
- 別 ROM・別ビルドのゴミデータを誤ロードしない
- API: `save_flash_sram_save()` / `save_flash_sram_load()`
- SD バックアップ用 API（`save_sram.c`）は将来のメニュー UI から呼び出し可能

### 10.5 セーブステート

- F2 で現在スロットへ保存、F3 でスロット切り替え（0〜9）、F4 でロード
- スロット 99（index 10）はスリープ専用（F1 スリープ時に自動保存・復帰時に自動ロード）
- Flash 書き込み時は `multicore_lockout_start/end_blocking()` で Core 1 を一時停止
- XIP 直読みでロードするためロックアウト不要

### 10.6 ストレージアイコン（右上 8×12px）

| 色 | 意味 |
|---|---|
| 白 | 読み込み中（SD / Flash 共通） |
| 青 | SD 書き込み中 |
| 黄 | Flash 書き込み中 |
| 消灯 | アイドル |

---

## 11. 省電力・スリープ仕様

### 11.1 基本方針

PicoCalcを携帯機として利用するため、スリープ・復帰機能を実装する。

### 11.2 初期段階

初期段階では明示的なメニュー操作により以下を行う。

- セーブRAM保存
- セーブステート保存
- 画面消灯
- 低消費電力状態へ移行

### 11.3 復帰

復帰時には、可能であれば直前のセーブステートを自動復元する。

---

## 12. マイルストーン

### Milestone 0: 開発環境構築 ✅

- Pico SDK 導入・CMake/Ninja ビルド確認・UF2 生成・PicoCalc 書き込み確認

### Milestone 1: PicoCalc 基本 I/O 🔄

- ✅ LCD 初期化（LovyanGFX / ILI9488）
- ✅ キーボード入力取得（STM32 I2C、Core 1 ポーリング）
- ✅ SD カード読み込み（FatFs_SPI）
- ⬜ スピーカー簡易出力（音声は Milestone 7 で対応済みだが単体テスト未実施）

### Milestone 2: GB コア組み込み ✅

- Peanut-GB 採用・ROM XIP 直読み・CPU 実行・映像バッファ取得

### Milestone 3: 画面表示 ✅

- 2x スケール（320×288）・LovyanGFX RGB565 差分描画・~60fps

### Milestone 4: 入力対応 ✅

- カーソルキー・WASD・`,`/`[`=A・`.`/`]`=B・BS=Start・Del=Select

### Milestone 5: 対象 ROM 起動 ✅

- 「カエルのために鐘は鳴る」タイトル表示・ゲーム開始・操作確認

### Milestone 6: セーブ対応 ✅

- SRAM 保存 / 読み込み（Flash XIP）・ゲーム内セーブ検出デバウンス・ROM タイトル照合検証

### Milestone 7: 音声対応 🔄

- ✅ GB APU 出力（minigb_apu）・PWM 12bit DMA IRQ・音程補正
- ⬜ 音量調整（メニュー UI 待ち）

### Milestone 8: GBC 対応 ⬜

- GBC ROM 起動・カラー表示・MBC 対応

### Milestone 9: 携帯機化 🔄

- ✅ セーブステート（F2/F3/F4）・スリープ（F1）・Flash メタデータ・SD オプション起動
- ⬜ メニュー UI（ROM リロード・セーブ削除・バックアップ・音量）
- ⬜ バッテリー残量表示

---

## 13. マルチコア構成（実装済み）

### Core 0（メインコア）

- GB エミュレーション実行（`gb_core_run_frame()`）
- ゲームフレームタイマー（59.727fps = 16743μs）
- APU サンプル生成 → SPSC リングバッファへ書き込み
- Flash 書き込み（`multicore_lockout_start/end_blocking()` で Core 1 を一時停止）
- ゲーム内セーブ検出・セーブステート操作

### Core 1（サブコア）

- LCD フレーム描画（LovyanGFX DMA、`lcd_gb_frame_delta()`）
- 音声 DMA IRQ 処理（リングバッファ → DMA バッファ → PWM）
- キーボード I2C ポーリング（`kbd_read_event()`）
- `multicore_lockout_victim_init()` で Flash 書き込み時の停止に対応

### コア間同期

| 機構 | 用途 |
|---|---|
| `mutex_t g_lcd_mutex` | LCD SPI バス排他（Core 0 でアイコン描画時） |
| `g_lcd_busy` フラグ（volatile） | Core 0→Core 1 フレーム転送通知 |
| `g_joypad` / `g_function_key`（volatile） | Core 1→Core 0 入力転送 |
| SPSC リングバッファ（g_afifo）| Core 0→Core 1 音声転送 |
| `multicore_lockout_*` | Flash 書き込み時の Core 1 停止 |

---

## 14. 技術的リスク

### 13.1 RAM不足

RP2350Aは520KB SRAMを持つが、フレームバッファを大きく持つと不足する可能性がある。

対策：

- フルフレームバッファを避ける
- ライン単位描画
- GB内部画面バッファのみ保持
- LCD転送バッファを小さくする

### 13.2 LCD転送速度

LCD転送がボトルネックになる可能性がある。

対策：

- DMA利用
- 差分描画
- フレームスキップ
- スケーリング方式の簡略化

### 13.3 GBC対応負荷

GBCはDMGより表示・MBC・互換性の負荷が増える。

対策：

- まずDMGを安定させる
- GBCは次段階にする
- 対象ROMを限定して検証する

### 13.4 PicoCalc固有仕様

PicoCalcのLCD、キーボード、SD、音声の接続仕様に依存する。

対策：

- platform層を分離
- PicoCalc依存コードを局所化
- A-2 ESP32-S3移植を見据える

---

## 15. 移植性

本プロジェクトは将来的にESP32-S3端末へ移植する可能性があるため、以下の層を分離する。

| 層 | 内容 |
|---|---|
| emu | GB/GBCエミュレーションコア |
| platform | ハードウェア依存初期化 |
| video | LCD表示 |
| input | キー入力 |
| audio | 音声出力 |
| storage | ROM/セーブ読み書き |
| ui | メニュー・設定画面 |

---

## 16. 初期実装方針

最初の実装では、完成度よりも起動確認を優先する。

優先順位：

1. PicoCalcでHello World表示
2. キー入力確認
3. SDカードからROM読み込み
4. GBコア組み込み
5. 音なしで画面表示
6. 「カエルのために鐘は鳴る」起動
7. 入力対応
8. セーブRAM対応
9. 音声対応
10. GBC対応

---

## 17. ライセンス・ROM取り扱い

ROMファイルはユーザーが所有するもののみを使用する。

ROM、BIOS、商用ゲームデータはリポジトリに含めない。

エミュレーションコアを外部プロジェクトから利用する場合、そのライセンスを確認し、リポジトリ内に明記する。

---

## 18. 当面の TODO（2026-05-25 時点）

### 優先度高

- [ ] ストレージアイコン色の実機確認（ゲーム内セーブ時の黄アイコンが青に見える問題の調査）
- [ ] メニュー UI 基本実装（Enter/Esc でオーバーレイ表示）
  - ROM リロード（`flash_meta_clear_rom()` → 再起動）
  - セーブデータ削除（`flash_meta_clear_sram()`）
  - SD バックアップ / リストア

### 優先度中

- [ ] バッテリー残量表示（キーボードコントローラ I2C レジスタ 0x0B）
- [ ] 音量調整（メニュー UI から PWM デューティ比を変更）

### 優先度低

- [ ] GBC 対応（Milestone 8）
- [ ] FPS 表示
- [ ] 複数 ROM サポート（ROM ファイル選択 UI）

---

## 19. エミュレーションコア選定方針

### 18.1 基本方針

本プロジェクトでは、ゼロからGame Boyエミュレータを実装するのではなく、既存の軽量GB/GBCエミュレーションコアを利用する。

ただし、PicoCalc依存コードとエミュレーションコアは分離し、将来的なESP32-S3系端末（A-2）への移植性を確保する。

---

### 18.2 想定構成

ソフトウェア構造は以下を目指す。

```
+-----------------------------+
| UI / Launcher               |
+-----------------------------+
| platform / video / audio    |
| input / storage             |
+-----------------------------+
| GB/GBC Emulator Core        |
+-----------------------------+
| RP2350 / Pico SDK           |
+-----------------------------+
```

---

### 18.3 候補工法一覧

| 工法 | 内容 | 長所 | 短所 | 想定用途 |
|---|---|---|---|---|
| 工法A | PicoCalc向け既存GB実装流用 | PicoCalc対応済み資産を利用可能 | コード整理・移植性に課題の可能性 | 初期検証 |
| 工法B | Peanut-GB / Walnut-CGB直接組込 | 軽量・移植性高い | PicoCalc I/Oを自前実装する必要 | 本命 |
| 工法C | pico-peanutGB系ベース | RP2350向け実績あり | LCD/UI置換が必要 | 有力候補 |
| 工法D | Rust系GB実装利用 | モダン構造 | SDK/言語方針と乖離 | 実験用 |

---

### 18.4 工法A: PicoCalc向け既存実装流用

候補：

- PicoCalc-GameBoy
- PocketPico
- PicoCalc forum上のGB emulator実装群

目的：

- PicoCalc上でのLCD
- キーボード
- Audio
- SD
- 電源管理

などの既存実装確認。

この工法では、まず「実際に動作しているPicoCalc実装」を調査し、ハードウェア初期化や周辺制御を把握する。

ただし、最終的には依存部分を分離する前提とする。

---

### 18.5 工法B: Peanut-GB / Walnut-CGB直接組込

最有力候補。

候補：

- Peanut-GB
- Walnut-CGB

特徴：

- 軽量
- シンプル
- 組み込み向き
- Cベース
- 移植性が高い

この工法では、PicoCalc依存コードを platform 層へ分離し、エミュレーションコアを独立させる。

将来的なA-2（ESP32-S3版）移植性を最も重視する場合、本工法が有力。

---

### 18.6 工法C: pico-peanutGB系ベース

RP2040 / RP2350向けGame Boyエミュレータ実装。

特徴：

- RP系向け最適化済み
- SDカード対応
- メニューUIあり
- GBC対応実績あり

ただし：

- HDMI出力前提の実装が含まれる場合がある
- PicoCalc LCD実装へ置換が必要
- UI設計を再整理する必要がある

本工法は「RP2350実装の参考資料」として有力。

---

### 18.7 工法D: Rust系GB実装

候補：

- Rust製GB emulator
- no_std系GB実装

特徴：

- モダンな構造
- 安全性
- 保守性

ただし：

- Pico SDK/CMake/Cベース構成との乖離
- RP2350組み込み実装難度
- ESP32-S3移植難度

が高いため、本プロジェクトでは優先度を低くする。

---

### 18.8 初期選定方針

初期方針は以下とする。

1. PicoCalc既存GB実装を調査
2. PicoCalc上でLCD/入力/Audio構成を把握
3. Peanut-GB / Walnut-CGBを評価
4. emu層を独立構成で実装
5. platform層を分離
6. A-2 ESP32-S3版移植を考慮した構造へ整理

---

### 18.9 推奨構成

現時点での推奨構成：

| 層 | 実装候補 |
|---|---|
| emu | Walnut-CGB または Peanut-GB |
| platform | PicoCalc専用実装 |
| build | Pico SDK + CMake |
| storage | FatFS + SD |
| video | PicoCalc LCD DMA転送 |
| audio | PWMベース |
| input | PicoCalc keyboard matrix |

---

### 18.10 技術的注目点

#### LCD転送

RP2350ではLCD転送がボトルネック化する可能性が高い。

以下を検討する。

- DMA転送
- 部分更新
- フレームスキップ
- RGB565変換最適化
- スケーリング簡略化

#### SRAM使用量

RP2350Aの520KB SRAMを超過しないよう注意する。

対策：

- フルスクリーンバッファ回避
- ラインバッファ化
- SaveStateサイズ管理

#### GBC対応

GBC対応では：

- VRAM増加
- カラーパレット
- MBC複雑化

などにより負荷が増える。

まずDMG安定動作を優先する。

---

### 18.11 将来的な共通化

本プロジェクトでは将来的に以下を共通化することを目指す。

```
共通:
  emu/
  ui/
  storage/

PicoCalc依存:
  platform_picocalc/
  video_picocalc/

ESP32-S3依存:
  platform_esp32/
  video_esp32/
```

これにより、B-2で開発したGB/GBCコアをA-2へ移植しやすくする。