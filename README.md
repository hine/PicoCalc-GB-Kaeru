# PicoCalc-GB-Kaeru

Game Boy emulator for [ClockworkPi PicoCalc](https://www.clockworkpi.com/picocalc), running on the RP2350A.

The name comes from the game it was built for: *For the Frog the Bell Tolls* (カエルのために鐘は鳴る).

> **Note:** This emulator was built solely to play *For the Frog the Bell Tolls*. No other games have been tested, and there is no intention to test them.

---

## Features

- Game Boy (DMG) emulation via [Peanut-GB](https://github.com/deltabeard/Peanut-GB)
- 160×144 → 320×288 2× scaled display (LovyanGFX, ~60 fps)
- 4 display palettes: DMG Green / Mono / Sepia / GB Pocket
- Audio output via PWM (Core 1 / DMA IRQ driven)
- ROM and save data stored in onboard Flash (SD card not required at runtime)
- Save states: 10 slots, stored in Flash
- Auto-save: SRAM written to Flash ~1 second after in-game save
- SD card backup/restore via in-game menu
- In-game menu (ESC key): palette, audio toggle, SD backup/restore, Flash clear

---

## Hardware

| Item | Details |
|---|---|
| Device | ClockworkPi PicoCalc |
| SoC | Raspberry Pi RP2350A |
| CPU | Dual-core Arm Cortex-M33 @ 150 MHz |
| SRAM | 520 KB |
| Flash | 16 MB |

---

## Flash Layout

```
0x000000 – 0x0FFFFF  ( 1 MB)  Firmware
0x100000 – 0x17FFFF  (512 KB) ROM image
0x180000 – 0x187FFF  ( 32 KB) SRAM save
0x188000 – 0x1C7FFF  (320 KB) Save states (10 slots × 32 KB)
```

---

## Key Bindings

| Key | Function |
|---|---|
| WASD / Arrow keys | D-Pad |
| `,` / `[` | A button |
| `.` / `]` | B button |
| Backspace / Enter | Start |
| Del | Select |
| F1 | Soft Reset |
| F2 | Save state (current slot) |
| F3 | Load state (current slot) |
| F4 | Change save slot (0–9) |
| ESC | Open/close menu |

---

## Build

### Requirements

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) (`PICO_SDK_PATH` must be set)
- CMake ≥ 3.13
- Ninja
- `gcc-arm-none-eabi`

See [DevelopmentEnvironment.md](DevelopmentEnvironment.md) for full setup instructions.

### Clone

```sh
git clone --recurse-submodules <repo-url> PicoCalc-GB-Kaeru
cd PicoCalc-GB-Kaeru
```

### Compile

```sh
cmake -S . -B build \
  -G Ninja \
  -DPICO_BOARD=pico2 \
  -DPICO_PLATFORM=rp2350 \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build -j$(nproc)
```

### Flash

Hold BOOTSEL on the RP2350 board and connect via USB, then copy the UF2:

```sh
cp build/picocalc_gb_kaeru.uf2 /media/$USER/RPI-RP2/
```

---

## ROM Handling

ROM files are **not included** and are excluded from the repository (`.gitignore`).  
Use only ROM images dumped from cartridges you legally own.

To write a ROM to Flash, place `kaeru.gb` in the `roms/` directory on the SD card and power on. The firmware detects it, copies it to Flash, and removes the file from the SD card.

---

## License

This project's original source code is licensed under the [MIT License](LICENSE).

### Third-party libraries (as git submodules)

| Library | License |
|---|---|
| [Peanut-GB](https://github.com/deltabeard/Peanut-GB) | MIT |
| [minigb_apu](https://github.com/deltabeard/Peanut-GB/tree/master/examples/sdl2/minigb_apu) | MIT |
| [LovyanGFX](https://github.com/lovyan03/LovyanGFX) | FreeBSD (2-clause BSD) |
| [no-OS-FatFS-SD-SPI-RPi-Pico](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico) | Apache 2.0 |
| FatFs (embedded in above) | BSD-style (ChaN) |

Each library retains its own license. See the respective `LICENSE` files under `lib/`.

---

---

# PicoCalc-GB-Kaeru（日本語）

[ClockworkPi PicoCalc](https://www.clockworkpi.com/picocalc)（RP2350A搭載）で動作する Game Boy エミュレータです。

プロジェクト名は、このエミュレータを作った目的のゲーム「カエルのために鐘は鳴る」に由来します。

> **注意:** このエミュレータは「カエルのために鐘は鳴る」をプレイするためだけに作りました。それ以外のゲームの動作確認はしていませんし、する予定もありません。

---

## 機能

- [Peanut-GB](https://github.com/deltabeard/Peanut-GB) による Game Boy（DMG）エミュレーション
- 160×144 → 320×288 の 2倍スケール表示（LovyanGFX、約60fps）
- 4種類のパレット: DMGグリーン / モノクロ / セピア / GBポケット
- PWM 音声出力（Core 1 / DMA IRQ 駆動）
- ROM・セーブデータをオンボード Flash に保存（実行時にSDカード不要）
- セーブステート: 10スロット（Flash 保存）
- 自動セーブ: ゲーム内セーブの約1秒後に Flash へ自動書き込み
- SDカードへのバックアップ／書き戻し（メニューから操作）
- ゲーム内メニュー（ESCキー）: パレット切替・音声ON/OFF・SDバックアップ・Flash全消去

---

## ハードウェア

| 項目 | 内容 |
|---|---|
| 本体 | ClockworkPi PicoCalc |
| SoC | Raspberry Pi RP2350A |
| CPU | Dual-core Arm Cortex-M33 @ 150 MHz |
| SRAM | 520 KB |
| Flash | 16 MB |

---

## Flash レイアウト

```
0x000000 – 0x0FFFFF  ( 1 MB)  ファームウェア
0x100000 – 0x17FFFF  (512 KB) ROM イメージ
0x180000 – 0x187FFF  ( 32 KB) SRAM セーブ
0x188000 – 0x1C7FFF  (320 KB) セーブステート（10スロット × 32 KB）
```

---

## キーバインド

| キー | 機能 |
|---|---|
| WASD / カーソルキー | 十字キー |
| `,` / `[` | A ボタン |
| `.` / `]` | B ボタン |
| Backspace / Enter | スタート |
| Del | セレクト |
| F1 | ソフトリセット |
| F2 | ステートセーブ（現在スロット） |
| F3 | ステートロード（現在スロット） |
| F4 | セーブスロット切替（0〜9） |
| ESC | メニュー開閉 |

---

## ビルド方法

詳細は [DevelopmentEnvironment.md](DevelopmentEnvironment.md) を参照してください。

```sh
# リポジトリのクローン（サブモジュール含む）
git clone --recurse-submodules <repo-url> PicoCalc-GB-Kaeru
cd PicoCalc-GB-Kaeru

# ビルド
cmake -S . -B build \
  -G Ninja \
  -DPICO_BOARD=pico2 \
  -DPICO_PLATFORM=rp2350 \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build -j$(nproc)

# 書き込み（BOOTSELボタンを押しながらUSB接続後）
cp build/picocalc_gb_kaeru.uf2 /media/$USER/RPI-RP2/
```

---

## ROM の取り扱い

ROM ファイルはリポジトリに含まれておらず、`.gitignore` で除外されています。  
**合法的に所有・吸い出したカートリッジの ROM のみ**を使用してください。

ROM を Flash に書き込むには、SD カードの `roms/` フォルダに `kaeru.gb` を置いて起動してください。  
ファームウェアが検出・Flash へコピー後、SD カードからファイルを削除します。

---

## ライセンス

本プロジェクトのオリジナルコードは [MIT ライセンス](LICENSE) で公開しています。

### 使用ライブラリ（謝辞）

本プロジェクトは以下の優れたオープンソースライブラリなしには実現しませんでした。開発者の皆さんに深く感謝します。

---

#### [Peanut-GB](https://github.com/deltabeard/Peanut-GB) — MIT License
*Mahyar Koshkouei 氏*

ヘッダオンリーで使えるシンプルかつ高品質な Game Boy エミュレーションコア。このプロジェクトのエミュレーション部分はほぼ Peanut-GB が担っています。組み込みターゲットへの移植性が非常に高く、RP2350A 上でも60fps が安定して出ます。Mahyar 氏の継続的なメンテナンスと丁寧なドキュメントに感謝します。

#### [minigb_apu](https://github.com/deltabeard/Peanut-GB/tree/master/examples/sdl2/minigb_apu) — MIT License
*Alex Baines 氏、Mahyar Koshkouei 氏*

Peanut-GB に同梱されている Game Boy APU（音声処理ユニット）実装です。GB の4チャンネル音源（矩形波×2・波形メモリ・ノイズ）を正確に再現しており、本プロジェクトの音声出力を支えています。

#### [LovyanGFX](https://github.com/lovyan03/LovyanGFX) — FreeBSD License
*lovyan03 氏*

RP2040/RP2350 をはじめとする組み込みマイコン向けの高速グラフィックスライブラリ。PicoCalc の ILI9341 LCD への描画をシンプルな API で実現できるのは LovyanGFX のおかげです。差分描画・DMA 転送・豊富なフォント対応など、実用的な機能が充実しています。lovyan03 氏の精力的な開発と日本語コミュニティへの貢献に感謝します。

#### [no-OS-FatFS-SD-SPI-RPi-Pico](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico) — Apache License 2.0
*Carl Kugler 氏*

Raspberry Pi Pico / Pico 2 向けの FatFs SPI SD カードドライバ。OS なしの環境でも安定して SD カードを読み書きできます。本プロジェクトでは ROM の初回書き込みおよびセーブデータのバックアップ用 SD アクセスに使用しています。

#### [FatFs](http://elm-chan.org/fsw/ff/00index_e.html) — BSD-style License
*ChaN 氏*

上記 no-OS-FatFS-SD-SPI-RPi-Pico に組み込まれている FAT ファイルシステムモジュール。長年にわたって組み込み開発者に愛用されてきた信頼性の高い実装です。

---

各ライブラリのライセンス全文は `lib/` 以下の `LICENSE` ファイルを参照してください。
