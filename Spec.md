# Spec.md

## 1. プロジェクト概要

RP2350A 搭載ボード + ClockworkPi PicoCalc 上で、「カエルの為に鐘は鳴る」（DMG Game Boy）を動かすための専用エミュレータ。

他のゲームの動作確認はしておらず、対応する予定もない。

---

## 2. 目的

PicoCalc 上で「カエルの為に鐘は鳴る」を快適にプレイできる状態にする。

具体的には以下を実装する（すべて達成済み）：

- GB DMG エミュレーション
- セーブ RAM 対応（Flash 保存・自動セーブ）
- セーブステート対応（10 スロット・Flash 保存）
- PicoCalc キーボードによる操作
- SD カードからの ROM 初回ロード（以降は Flash から直読み）
- LCD への 2x スケール表示（320×288）
- 音声出力（PWM 12bit DMA IRQ）
- ゲーム内メニュー（パレット・音声・SD バックアップ・Flash クリア）

---

## 3. 対象ハードウェア

### 3.1 本体

- ClockworkPi PicoCalc

### 3.2 SoC ボード

PicoCalc 内部の Raspberry Pi Pico 互換ピンに装着する RP2350A 搭載ボードを使用する。

| 項目 | 内容 |
|---|---|
| SoC | Raspberry Pi RP2350A |
| CPU | Dual-core Arm Cortex-M33 |
| 最大クロック | 150 MHz |
| SRAM | 520 KB |
| Flash | 16 MB |
| PWM | 16ch |

### 3.3 使用する PicoCalc 機能

- LCD（ILI9488 320×320、SPI1）
- キーボード（STM32 I2C コントローラ、GP6/7、アドレス 0x1F）
- microSD（SPI0、GP16-19）
- スピーカー（GP26/27、アンプ直結 PWM 専用）
- バッテリー（UI は "--" 表示。STM32 ファームウェアが 0x0B 未実装のため読み取り不可）

---

## 4. 開発環境

```sh
cmake -S . -B build \
  -G Ninja \
  -DPICO_BOARD=pico2 \
  -DPICO_PLATFORM=rp2350 \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build -j$(nproc)
```

詳細は [DevelopmentEnvironment.md](DevelopmentEnvironment.md) を参照。

---

## 5. ソフトウェア構成

### 5.1 ディレクトリ構成

```
PicoCalc-GB-Kaeru/
  CMakeLists.txt
  compat/hardware/rtc.h    # RP2350 で廃止された hardware_rtc のスタブ
  src/
    main.c
    video/
      video_lcd.h          # LCD API
      video_lgfx.cpp       # LovyanGFX 実装（差分描画・ステータスバー・メニュー描画）
      lgfx_config.hpp      # LovyanGFX パネル設定
    input/
      input_keyboard.c/h   # STM32 キーボードコントローラ I2C ポーリング
    audio/
      audio_pwm.c/h        # PWM 12bit + DMA IRQ ダブルバッファ（Core 0）
    storage/
      hw_config.c          # FatFs_SPI ハードウェア設定
      storage_sd.c/h       # SD マウント/アンマウント
      rom_flash.c/h        # ROM を Flash XIP に書き込み・検証
      save_flash.c/h       # SRAM セーブ・セーブステートを Flash に保存
      flash_meta.c/h       # Flash メタデータ（ROM 有効フラグ・SRAM 検証）
      save_sram.c/h        # SD バックアップ用 SRAM 保存
      save_state.c/h       # SD バックアップ用ステート保存
    menu/
      menu.c/h             # メニューロジック（項目・ナビゲーション・アクション）
  emu/
    gb/
      gb_core.c/h          # Peanut-GB ラッパー（ROM XIP・APU・セーブステート統合）
  lib/
    peanut-gb/             # GB エミュレーションコア（ヘッダオンリー）
    LovyanGFX/             # LCD ドライバライブラリ
    no-OS-FatFS-SD-SPI-RPi-Pico/  # FatFS + SPI SD ドライバ
```

### 5.2 Git 管理方針

ROM ファイル・セーブデータはリポジトリに含めない（`.gitignore` で除外）。

---

## 6. エミュレーション仕様

### 6.1 実装済み

- Game Boy / DMG（Peanut-GB）
- ROM を Flash XIP で提供（SD は初回ロード時のみ）
- MBC 対応（Peanut-GB が処理）
- SRAM セーブ対応（Flash 保存・ゲーム内セーブ検出デバウンス）
- セーブステート対応（F2/F3/F4、Flash 10 スロット）

### 6.2 対象外（恒久）

- Game Boy Color / GBC
- Game Boy Advance
- 通信ケーブル・赤外線通信
- RTC 付き特殊カートリッジ
- 他ゲームタイトルの動作保証

---

## 7. 表示仕様

| 項目 | 内容 |
|---|---|
| LCD | PicoCalc 内蔵 ILI9488（320×320、SPI1） |
| 色並び順 | BGR（MADCTL 0x48 bit3=1）→ LovyanGFX `rgb_order = false` 必須 |
| GB 画面サイズ | 160×144 → 2x スケール = 320×288 |
| 表示位置 | y=16〜303（上下各 16px をステータスバーに確保） |
| 描画方式 | LovyanGFX RGB565 差分描画（変化ピクセルのみ転送） |
| パレット | DMGグリーン / モノクロ / セピア / GBポケット（メニューから切替可） |
| フレームレート | ~60fps（Core 1 で非同期描画、Core 0 はノンブロッキング） |

### 7.1 ステータスバー

```
y=0〜15  (16px) 上部:
  x=0〜49    ステータステキスト（操作メッセージ・スロット番号等）
  x=52〜281  F キーバインド（F1=Reset / F2=Save / F3=Load / F4=Slot）
  x=282〜305 バッテリー残量（"--" 固定。STM32 ファームウェア未対応）
  x=310〜317 ストレージアイコン（白=読込・青=SD書込・黄=Flash書込）
y=16〜303  GB 画面（2x スケール 320×288）
y=304〜319 (16px) 下部: キーヒント
```

---

## 8. 入力仕様

### 8.1 ゲームキーマッピング

| GB 入力 | 主キー | 副キー |
|---|---|---|
| Up | カーソル↑ | W |
| Down | カーソル↓ | S |
| Left | カーソル← | A |
| Right | カーソル→ | D |
| A ボタン | `,` | `[` |
| B ボタン | `.` | `]` |
| Start | Backspace | Enter |
| Select | Del | — |

### 8.2 システムキー（ゲームに渡さない）

| キー | 機能 |
|---|---|
| F1 | ソフトリセット（GB 再初期化 + Flash SRAM リロード） |
| F2 | セーブステート保存（現在スロット） |
| F3 | セーブステートロード（現在スロット） |
| F4 | セーブスロット切り替え（0〜9 サイクル） |
| ESC | メニュー開く |

---

## 9. 音声仕様

| 項目 | 内容 |
|---|---|
| 出力方式 | PWM（GP26/GP27、PicoCalc アンプ直結） |
| 解像度 | 12bit（AUDIO_WRAP=4095） |
| サンプリング周波数 | ≈32767 Hz（TIMER_WRAP=4578 @ 150MHz、誤差 0.004%） |
| APU 生成レート | 32769 Hz（AUDIO_SAMPLE_RATE=32800 → AUDIO_SAMPLES=549 サンプル/フレーム） |
| バッファ | DMA ダブルバッファ（549 サンプル/バッファ、~33ms レイテンシ） |
| 駆動方式 | DMA IRQ 駆動（Core 0 で処理） |
| APU コア | minigb_apu（Peanut-GB 同梱、S16 ステレオ） |

**設計上の注意:**
- I2S DAC 非搭載のため I2S 化は不可（GP26/27 はアンプ直結 PWM 専用）
- DMA IRQ は Core 0 に登録すること。Core 1 では `multicore_lockout` で停止するため、Flash 書き込み中にハードウェアチェーン DMA が TRANS_COUNT=0 で暴走し「ザザッ」ノイズが発生する
- IRQ ハンドラは `__not_in_flash_func` で SRAM に配置し、XIP 無効中でも実行可能にすること

---

## 10. ストレージ仕様

### 10.1 基本方針

SD カードはカード破損リスクがあるため、**ゲームデータの永続保存には Flash を使用する**。SD は初回 ROM ロード時と SD バックアップ UI のみで使用する。

### 10.2 Flash レイアウト

```
アドレス      サイズ    用途
0x000000     1 MB      ファームウェア
0x100000   512 KB      ROM データ（XIP 直読み）
0x180000    32 KB      SRAM セーブ
0x188000   320 KB      セーブステート × 10 スロット（各 32 KB）
0x1C8000     4 KB      Flash メタデータ
```

### 10.3 ROM 管理

- 初回起動時に SD カードの `roms/kaeru.gb` を Flash に書き込む
- 2 回目以降は Flash メタデータの ROM 有効フラグを確認し、SD なしで起動可能
- SD 挿入時は ROM タイトル照合を行い、差分があれば再フラッシュ

### 10.4 SRAM セーブ

- ゲームが cart RAM に書き込んだ後、~1 秒（60 フレーム）のデバウンスを経て自動保存
- Flash メタデータにマジック + ROM タイトル（11 バイト）を記録し、ロード時に検証
- 別 ROM・別ビルドのゴミデータを誤ロードしない

### 10.5 セーブステート

- F2 で現在スロットへ保存、F3 でロード、F4 でスロット切り替え（0〜9）
- Flash 書き込み時は `multicore_lockout_start/end_blocking()` で Core 1 を一時停止
- XIP 直読みでロードするためロックアウト不要

---

## 11. マルチコア構成

### Core 0（メインコア）

- GB エミュレーション実行（`gb_core_run_frame()`）
- ゲームフレームタイマー（59.727fps = 16743μs）
- APU サンプル生成 → SPSC リングバッファへ書き込み
- 音声 DMA IRQ 処理（リングバッファ → DMA バッファ → PWM）
- Flash 書き込み（`multicore_lockout_start/end_blocking()` で Core 1 を一時停止）

> **DMA IRQ を Core 0 に置く理由:** Core 1 では `multicore_lockout` で停止する。Flash 書き込み中の割り込み禁止ウィンドウ（~100ms）でシングルチャンネル DMA が停止しても短い無音になるだけで、ハードウェアチェーン DMA のような TRANS_COUNT=0 暴走は発生しない。

### Core 1（サブコア）

- LCD フレーム描画（LovyanGFX DMA、`lcd_gb_frame_delta()`）
- キーボード I2C ポーリング（`kbd_read_event()`）
- `multicore_lockout_victim_init()` で Flash 書き込み時の停止に対応

### コア間同期

| 機構 | 用途 |
|---|---|
| `mutex_t g_lcd_mutex` | LCD SPI バス排他 |
| `g_lcd_busy` フラグ（volatile） | Core 0→Core 1 フレーム転送通知 |
| `g_menu_active` フラグ（volatile） | メニュー中のフレーム描画抑制 |
| `g_joypad` / `g_function_key`（volatile） | Core 1→Core 0 入力転送 |
| SPSC リングバッファ（g_afifo）| APU 出力 → DMA IRQ 音声転送 |
| `multicore_lockout_*` | Flash 書き込み時の Core 1 停止 |

---

## 12. マイルストーン

| # | 内容 | 状態 |
|---|---|---|
| 0 | 開発環境構築 | ✅ |
| 1 | PicoCalc 基本 I/O（LCD・KB・SD・音声） | ✅ |
| 2 | GB コア組み込み（Peanut-GB） | ✅ |
| 3 | 画面表示（2x スケール・~60fps） | ✅ |
| 4 | 入力対応 | ✅ |
| 5 | 対象 ROM 起動（カエルの為に鐘は鳴る） | ✅ |
| 6 | セーブ対応（SRAM・Flash・自動セーブ） | ✅ |
| 7 | 音声対応（PWM 12bit DMA IRQ） | ✅ |
| 8 | GBC 対応 | ⬜ 予定なし |
| 9 | 携帯機化（メニュー・セーブステート・SD バックアップ） | ✅ |

---

## 13. 既知の制限・未解決事項

| 項目 | 内容 |
|---|---|
| バッテリー残量 | STM32 ファームウェアが I2C 0x0B を未実装（NACK）。"--" 表示で運用。clockworkpi/PicoCalc#20 参照 |
| LCD バックライト（I2C 0x0A） | STM32 ファームウェアが 0x0A レジスタを未実装とみられる。メニューから削除済み（Flash 設定形式は維持） |
| FPS 表示 | 未実装 |
| RTC / タイムスタンプ | RP2350 で hardware_rtc が廃止。`compat/hardware/rtc.h` の no-op スタブで対処。FatFS のファイル日時は 1980-01-01 固定 |

---

## 14. ライセンス

本プロジェクトのオリジナルコードは MIT ライセンスで公開。詳細は [LICENSE](LICENSE) を参照。

依存ライブラリのライセンス一覧は [README.md](README.md) を参照。
