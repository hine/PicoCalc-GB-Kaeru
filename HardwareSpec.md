# PicoCalc ハードウェア仕様

調査日: 2026-05-22

## 1. ピン割り当て一覧

| 機能 | 信号 | GPIO | インターフェース |
|---|---|---|---|
| LCD | SCK | GP10 | SPI1 |
| LCD | MOSI | GP11 | SPI1 |
| LCD | MISO | GP12 | SPI1 |
| LCD | CS | GP13 | SPI1 |
| LCD | DC | GP14 | GPIO |
| LCD | RST | GP15 | GPIO |
| キーボード | SDA | GP6 | I2C1 |
| キーボード | SCL | GP7 | I2C1 |
| SD | MISO | GP16 | SPI0 |
| SD | CS | GP17 | SPI0 |
| SD | SCK | GP18 | SPI0 |
| SD | MOSI | GP19 | SPI0 |
| SD | Detect | GP22 | GPIO |
| 音声 L | PWM | GP26 | PWM |
| 音声 R | PWM | GP27 | PWM |
| PSRAM | CS | GP20 | — |
| PSRAM | SCK | GP21 | — |
| PSRAM | MOSI | GP2 | — |
| PSRAM | MISO | GP3 | — |

---

## 2. LCD

### 2.1 概要

| 項目 | 内容 |
|---|---|
| パネル解像度 | 320 × 480 |
| 実効表示解像度 | **320 × 320**（下部 160 行はブランク） |
| LCDコントローラ | 初期ロット: **ILI9488** / 最新ロット: **ST7365P** |
| ST7365P互換性 | ILI9488 と 99% コマンド互換（3-bit カラーモード非対応） |
| インターフェース | **4線式 SPI1** |
| SPI クロック | 仕様上 ~15MHz だが 25〜75MHz での動作報告あり |
| ピクセルフォーマット | 18-bit（RGB666） |
| バックライト制御 | STM32 キーコントローラ経由 I2C（I2C レジスタ 0x0A bit7=1） |

### 2.2 初期化シーケンス（ILI9488 / ST7365P 共通）

公式 helloworld サンプル（`clockworkpi/PicoCalc` リポジトリ内 `lcdspi.c`）に基づく。

1. GPIO 設定 + SPI1 を 25MHz で初期化
2. RST でハードウェアリセット
3. コマンドシーケンス送信：
   - Positive/Negative Gamma Control (0xE0, 0xE1)
   - Power Control 1/2 (0xC0, 0xC1)
   - VCOM Control (0xC5)
   - Memory Access Control (0x36, 値 0x48 = MX + BGR)
   - Pixel Interface Format (0x3A, 値 0x66 = 18-bit SPI)
   - Frame Rate Control (0xB1, 0xA0)
   - Display Inversion On (0x21)
   - Function Control (0xB6)
   - Sleep Out (0x11) → 120ms wait
   - Display On (0x29) → 120ms wait

### 2.3 GB 表示への適用

GB の画面解像度は 160 × 144。PicoCalc LCD（320 × 320）への表示方法：

| 方法 | スケール | 表示サイズ |
|---|---|---|
| 等倍 | 1x | 160 × 144（中央寄せ余白あり） |
| 整数2倍 | 2x | 320 × 288（横いっぱい、縦余白あり） |
| アスペクト比維持最大 | 2x | 320 × 288 が現実的な最大 |

DMA 転送の活用を検討する（RP2350 の DMA コントローラ使用）。

### 2.4 参照ソース

- 公式ドライバ: `clockworkpi/PicoCalc` → `Code/picocalc_helloworld/lcdspi/`
- データシート: 同リポジトリ内 `ST7365P_SPEC_V1.0.pdf`

---

## 3. キーボード

### 3.1 概要

| 項目 | 内容 |
|---|---|
| キー数 | 67キー（QWERTY配列 + ファンクションキー） |
| マトリクス | 7行 × 8列 |
| コントローラ | **STM32F103R8T6**（ARM Cortex-M3） |
| 接続方式 | **I2C1** |
| I2C アドレス | **0x1F** |
| I2C 速度 | **10kHz 固定**（これより速いと通信不安定） |
| 割り込み | なし（ポーリング専用） |

### 3.2 I2C レジスタマップ

| レジスタ | アドレス | 説明 |
|---|---|---|
| REG_ID_FIF | 0x09 | キーボード FIFO 読み出し |
| バックライト | 0x0A (bit7=1) | バックライト輝度 (0〜255) |
| バッテリー | 0x0B | バッテリー残量 |

### 3.3 キー読み出し手順

```
1. 0x09 を I2C Write
2. 16ms 待機
3. 2バイト I2C Read

16-bit データフォーマット:
  [15:8] = ASCII キーコード
  [0]    = キー押下フラグ (1=押下, 0=解放)

特殊値:
  0x7E02 = Ctrl キー押下
  0x7E03 = Ctrl キー解放
```

### 3.4 GB 向けキーマッピング（初期案）

| GB 入力 | PicoCalc キー | キーコード |
|---|---|---|
| Up | カーソル上 | 0xB5 |
| Down | カーソル下 | 0xB6 |
| Left | カーソル左 | 0xB4 |
| Right | カーソル右 | 0xB7 |
| A | A キー | 0x61 |
| B | B キー | 0x62 |
| Start | Enter | 0x0A |
| Select | Space | 0x20 |
| Menu | Esc | 0xB1 |

### 3.5 参照ソース

- 公式ドライバ: `clockworkpi/PicoCalc` → `Code/picocalc_helloworld/i2ckbd/`

---

## 4. SDカード

### 4.1 概要

| 項目 | 内容 |
|---|---|
| インターフェース | **SPI0** |
| ファイルシステム | FAT32 |
| ライブラリ | **FatFS**（推奨: `no-OS-FatFS-SD-SPI-RPi-Pico` by carlk3） |
| カード挿入検出 | GP22（GPIO） |

### 4.2 パーティション構成（参考）

| パーティション | 種別 | 用途 |
|---|---|---|
| 1 | FAT32 | アプリ・ROM・データ |
| 2 | Linux 32MB | FUZIX用（本プロジェクトでは不使用） |

### 4.3 本プロジェクトでの使用予定

```
/roms/kaeru.gb       ← ROMファイル
/saves/kaeru.sav     ← セーブRAM
/saves/kaeru.slot0.state  ← セーブステート
```

---

## 5. 音声

| 項目 | 内容 |
|---|---|
| 出力方式 | **PWM**（初期実装） / I2S（高音質版） |
| 左チャンネル | GP26 |
| 右チャンネル | GP27 |

PicoCalc-GameBoy (jblanked) は I2S 44.1kHz 16-bit ステレオを使用。
初期実装は PWM で対応し、後で I2S に移行する。

---

## 6. 既存 GB エミュレータ実装（参考）

本プロジェクトの先行実装として以下が存在し、いずれも **Peanut-GB** エンジンを採用。

| プロジェクト | URL | エンジン | 速度 | 特記 |
|---|---|---|---|---|
| PicoCalc-GameBoy | https://github.com/jblanked/PicoCalc-GameBoy | Peanut-GB | 55〜70fps | RP2350対応、I2S音声あり |
| Picocalc_GBEmu | https://github.com/quanliew28/Picocalc_GBEmu | Peanut-GB | 14fps（改善予定） | ROM埋め込み方式 |
| PocketPico | https://github.com/TheKiwil/PocketPico | Peanut-GB | — | フォーク版、セーブ未実装 |
| Picoware | https://github.com/jblanked/Picoware | Peanut-GB他 | — | 総合ファームウェア |

→ **Peanut-GB** が PicoCalc + RP2350 向け GB エミュレーションの事実上の標準。

---

## 7. 情報ソース

- [clockworkpi/PicoCalc (公式)](https://github.com/clockworkpi/PicoCalc)
- [DeepWiki: Main Board](https://deepwiki.com/clockworkpi/PicoCalc/2.1-main-board)
- [DeepWiki: Display and Audio](https://deepwiki.com/clockworkpi/PicoCalc/2.3-display-and-audio)
- [DeepWiki: Programming Guide](https://deepwiki.com/clockworkpi/PicoCalc/5-programming-guide)
- [DeepWiki: SD Card](https://deepwiki.com/clockworkpi/PicoCalc/4.6-sd-card-and-multi-boot-system)
- [フォーラム: GPIO ピン割り当て](https://forum.clockworkpi.com/t/gpio-for-pico-calc-how-to-make-firmware-for-pico-calc/20905)
- [フォーラム: ILI9488 ピン設定](https://forum.clockworkpi.com/t/ili9488-pin-configuration/17547)
- [フォーラム: ST7365P LCD](https://forum.clockworkpi.com/t/new-lcd-screen-st7365p-in-recent-picocalc-commit/17649)
