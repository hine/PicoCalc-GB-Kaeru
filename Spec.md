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
      audio_pwm.c/h        # PWM 12bit + DMA IRQ ダブルバッファ（Core 0）
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

### 6.1 実装済み

- Game Boy / DMG（Peanut-GB）
- ROM を Flash XIP で提供（SD は初回ロード時のみ）
- MBC 対応（Peanut-GB が処理）
- SRAM セーブ対応（Flash 保存・ゲーム内セーブ検出）
- セーブステート対応（F2/F3/F4、Flash 11 スロット）

### 6.2 次段階

- Game Boy Color / GBC（Walnut-CGB または Peanut-GB GBC モード）

### 6.3 対象外（恒久）

- Game Boy Advance
- 通信ケーブル・赤外線通信
- RTC 付き特殊カートリッジ（ポケモン金銀等）
- 完全なタイミング互換性・全市販 ROM の完全動作

---

## 7. 表示仕様

### 7.1 実装済み構成

| 項目 | 内容 |
|---|---|
| LCD | PicoCalc 内蔵 ILI9488（320×320、SPI1） |
| GB 画面サイズ | 160×144 → 2x スケール = 320×288 |
| 表示位置 | y=16〜303（上下各 16px をステータスバーに確保） |
| 描画方式 | LovyanGFX RGB565 差分描画（変化ピクセルのみ転送） |
| パレット | DMG クラシックグリーン 4色（RGB888 → RGB565 変換済み） |
| フレームレート | ~60fps（Core 1 で非同期描画、Core 0 はノンブロッキング） |

### 7.2 ステータスバー

```
y=0〜15  (16px) 上部: ストレージアイコン（右端 8×12px）・将来: ROM名 等
y=16〜303       GB 画面エリア（2x スケール 320×288）
y=304〜319(16px) 下部: キーヒント（例: ,[=A  .]=B  BS=Sta  Del=Sel）
```

### 7.3 将来追加予定の UI 要素

- FPS 表示・バッテリー残量（上部ストリップ）
- メニューオーバーレイ（Enter/Esc で表示）

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
| サンプリング周波数 | ≈32767 Hz（TIMER_WRAP=4578 @ 150MHz、GB 正規 32768 Hz との誤差 0.004%） |
| APU 生成レート | 32769 Hz（AUDIO_SAMPLE_RATE=32800 → AUDIO_SAMPLES=549 サンプル/フレーム） |
| バッファ | DMA ダブルバッファ（549 サンプル/バッファ、~33ms レイテンシ） |
| 駆動方式 | DMA IRQ 駆動（Core 0 で処理、ポーリング不要） |
| APU コア | minigb_apu（Peanut-GB 同梱、S16 ステレオ） |
| データ経路 | Core 0 メインループ: APU → SPSC リングバッファ → Core 0 DMA IRQ: DMA → PWM |

### 9.2 設計上の注意

- I2S DAC 非搭載のため I2S 化は不可（GP26/27 はアンプ直結 PWM 専用）
- RP2350 デフォルトクロックは 150MHz（125MHz 前提の TIMER_WRAP は音程ズレを生じる）
- PWM キャリア周波数 = 150MHz / 4096 ≈ 36.6kHz（可聴域外）
- DMA IRQ は Core 0 に登録すること。Core 1 では `multicore_lockout` で停止するため、Flash 書き込み中に TRANS_COUNT=0 のハードウェアチェーン DMA が暴走して「ザザッ」ノイズが発生する
- AUDIO_SAMPLE_RATE=32800 に設定することで AUDIO_SAMPLES=549（int 切り捨て補正）となり、テンポ誤差を 0.117% → 0.004% に抑制する

### 9.3 備考

- 音量調整は PicoCalc 本体の物理ボリュームで対応するため、ソフトウェア実装は不要

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

### 11.1 実装済み構成

| 操作 | 動作 |
|---|---|
| F1（ゲーム中） | スリープ移行（go_dormant）|
| F1（スリープ中） | 復帰確認・ゲーム再開 |

### 11.2 スリープシーケンス（F1 押下時）

1. Core 1 を `multicore_reset_core1()` で停止
2. セーブステートを Flash スロット 99（スリープ専用）に保存
3. SRAM セーブを Flash に保存
4. キーバックライト消灯
5. `frame_timer` / 音声 DMA 停止
6. RP2350 dormant（AON タイマー 10 秒後起床、LPOSC 駆動）
7. 起床後 ROM ブートローダから再起動

### 11.3 復帰シーケンス

1. 再起動時に `powman_hw->scratch[0] == SLEEP_MAGIC` を確認
2. キーボード初期化後、F1 入力を最大 500ms 待つ
3. F1 が押されたら `RESUME_MAGIC` をセットして通常起動フローへ
4. 通常起動完了後、スロット 99 からステートをロードしてゲーム再開
5. F1 以外 → 再度 dormant（電源断と同等）

### 11.4 制約

- dormant 中は SRAM 電源が切れるため、セーブは Flash 必須
- RTC 非搭載のため復帰後の経過時間は不明

---

## 12. マイルストーン

### Milestone 0: 開発環境構築 ✅

- Pico SDK 導入・CMake/Ninja ビルド確認・UF2 生成・PicoCalc 書き込み確認

### Milestone 1: PicoCalc 基本 I/O ✅

- ✅ LCD 初期化（LovyanGFX / ILI9488）
- ✅ キーボード入力取得（STM32 I2C、Core 1 ポーリング）
- ✅ SD カード読み込み（FatFs_SPI）
- ✅ スピーカー出力（Milestone 7 で PWM 12bit DMA IRQ として完了）

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

### Milestone 7: 音声対応 ✅

- ✅ GB APU 出力（minigb_apu）・PWM 12bit DMA IRQ・音程補正
- ✅ Flash 書き込み中ノイズ解消・テンポ修正
- ℹ️ 音量調整は PicoCalc 本体の物理ボリュームで対応（ソフト実装不要）

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
- APU サンプル生成 → SPSC リングバッファへ書き込み（producer）
- 音声 DMA IRQ 処理（リングバッファ → DMA バッファ → PWM）（consumer、IRQ として preempt）
- Flash 書き込み（`multicore_lockout_start/end_blocking()` で Core 1 を一時停止）
- ゲーム内セーブ検出・セーブステート操作

> **DMA IRQ を Core 0 に置く理由:** Core 1 では `multicore_lockout` で停止する。また、Flash 書き込み中の割り込み禁止ウィンドウ（~100ms）でシングルチャンネル DMA が停止しても短い無音になるだけで、ハードウェアチェーン DMA のような TRANS_COUNT=0 暴走（「ザザッ」）は発生しない。

### Core 1（サブコア）

- LCD フレーム描画（LovyanGFX DMA、`lcd_gb_frame_delta()`）
- キーボード I2C ポーリング（`kbd_read_event()`）
- `multicore_lockout_victim_init()` で Flash 書き込み時の停止に対応

### コア間同期

| 機構 | 用途 |
|---|---|
| `mutex_t g_lcd_mutex` | LCD SPI バス排他（Core 0 でアイコン描画時） |
| `g_lcd_busy` フラグ（volatile） | Core 0→Core 1 フレーム転送通知 |
| `g_joypad` / `g_function_key`（volatile） | Core 1→Core 0 入力転送 |
| SPSC リングバッファ（g_afifo）| Core 0 メインループ（producer）→ Core 0 DMA IRQ（consumer）音声転送 |
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

## 16. 初期実装方針（完了）

最初の実装では完成度より起動確認を優先した。以下はすべて実機確認済み（Milestone 0〜7）。

1. ✅ PicoCalc で Hello World 表示
2. ✅ キー入力確認
3. ✅ SD カードから ROM 読み込み → Flash XIP に移行
4. ✅ GB コア組み込み（Peanut-GB）
5. ✅ 音なしで画面表示 → LovyanGFX 2x 差分描画
6. ✅ 「カエルのために鐘は鳴る」起動・操作
7. ✅ 入力対応（WASD・`[`/`]`・BS/Del）
8. ✅ セーブ RAM 対応（Flash、ゲーム内セーブ検出）
9. ✅ 音声対応（PWM 12bit DMA IRQ）
10. ⬜ GBC 対応（Milestone 8、未着手）

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

## 19. エミュレーションコア（採用決定）

### 19.1 採用コア

**Peanut-GB**（ヘッダオンリー C 実装）を採用。

- 軽量・移植性高い・RP2350 実績あり
- GBC 対応は Walnut-CGB（または Peanut-GB GBC モード）で Milestone 8 に評価
- PicoCalc 依存コードは `emu/` から分離し将来の ESP32-S3 移植（A-2）に対応

### 19.2 実装済みレイヤー構成

| 層 | 実装 |
|---|---|
| emu | Peanut-GB + minigb_apu（`emu/gb/gb_core.c`） |
| video | LovyanGFX / ILI9488（`src/video/video_lgfx.cpp`） |
| audio | PWM 12bit DMA IRQ（`src/audio/audio_pwm.c`） |
| input | STM32 I2C キーボード（`src/input/input_keyboard.c`） |
| storage | FatFS + SD + Flash XIP（`src/storage/`） |
| build | Pico SDK + CMake + Ninja |

### 19.3 移植性方針

将来的な A-2（ESP32-S3）移植に備え、`emu/` と `src/` を分離した構造を維持する。