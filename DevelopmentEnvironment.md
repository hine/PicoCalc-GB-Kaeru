# PicoCalc GB/GBC Project - Development Environment Setup

## 1. 概要

本ドキュメントでは、RP2350A + PicoCalc を用いた Game Boy / Game Boy Color 互換機開発環境の構築方法をまとめる。

対象プロジェクト：

- B-2: RP2350 + PicoCalc を用いた GB/GBC 互換機

開発方針：

- CLIベース開発
- Codex / Claude Code などAI支援開発を前提
- Pico SDK + CMake + Ninja を採用
- VSCode は補助用途
- compile_commands.json を生成し、AI補完を強化

対象OS：

- Ubuntu 24.04 LTS
- WSL2 Ubuntu 24.04
- Debian系Linux

---

# 2. 必要パッケージ

まずシステムパッケージを導入する。

```
sudo apt update

sudo apt install -y \
  git \
  cmake \
  ninja-build \
  gcc-arm-none-eabi \
  libnewlib-arm-none-eabi \
  build-essential \
  pkg-config \
  python3 \
  python3-pip \
  python3-venv \
  python3-setuptools \
  python3-wheel \
  unzip \
  wget \
  tar \
  minicom \
  picocom
```

---

# 3. 作業ディレクトリ作成

```
mkdir -p ~/dev
cd ~/dev
```

---

# 4. Pico SDK導入

RP2350は Raspberry Pi Pico SDK を利用する。

```
cd ~/dev

git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
```

環境変数を設定する。

```
echo 'export PICO_SDK_PATH=$HOME/dev/pico-sdk' >> ~/.bashrc
source ~/.bashrc
```

確認：

```
echo $PICO_SDK_PATH
```

---

# 5. picotool導入

UF2書き込みやデバッグ補助に使用する。

```
sudo apt install libusb-1.0-0-dev

cd ~/dev

git clone https://github.com/raspberrypi/picotool.git

cd picotool

mkdir build
cd build

cmake ..
make -j$(nproc)

sudo make install
```

確認：

```
picotool version
```

---

# 6. プロジェクト作成

作業ディレクトリを作成する。（もしくはgithubからcloneする）

```
cd ~/Projects/

mkdir PicoCalc-GB-Kaeru
cd PicoCalc-GB-Kaeru
```

---

# 7. 初期ディレクトリ構成

```
mkdir -p \
  src/platform \
  src/video \
  src/input \
  src/audio \
  src/storage \
  src/system \
  emu/gb \
  roms \
  saves \
  tools
```

---

# 8. .gitignore作成

```
cat << 'EOF' > .gitignore
build/
*.uf2
*.elf
*.bin
*.map
compile_commands.json

roms/*.gb
roms/*.gbc

saves/*
*.sav
*.state
EOF
```

---

# 9. CMakeLists.txt 作成

```
cat << 'EOF' > CMakeLists.txt
cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(picocalc_gb C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(picocalc_gb
    src/main.c
)

target_link_libraries(picocalc_gb
    pico_stdlib
)

pico_add_extra_outputs(picocalc_gb)

pico_enable_stdio_usb(picocalc_gb 1)
pico_enable_stdio_uart(picocalc_gb 0)

EOF
```

---

# 10. 最小main.c作成

```
cat << 'EOF' > src/main.c
#include <stdio.h>
#include "pico/stdlib.h"

int main()
{
    stdio_init_all();

    while (true)
    {
        printf("Hello PicoCalc GB\n");
        sleep_ms(1000);
    }
}
EOF
```

---

# 11. ビルド

```
cd ~/Projects/PicoCalc-GB-Kaeru

cmake -S . -B build \
  -G Ninja \
  -DPICO_BOARD=pico2 \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build
```

生成物：

```
build/picocalc_gb.uf2
```

---

# 12. PicoCalcへの書き込み

## 12.1 BOOTSELモード

RP2350ボードをBOOTSELモードでUSB接続する。

接続後、USBマスストレージとして認識される。

---

## 12.2 UF2コピー

```
cp build/picocalc_gb.uf2 /media/$USER/RPI-RP2/
```

自動的に再起動する。

---

# 13. シリアル確認

デバイス確認：

```
dmesg | tail
```

シリアル接続：

```
picocom -b 115200 /dev/ttyACM0
```

終了：

```
Ctrl-A
Ctrl-X
```

---

# 14. compile_commands.json

AI支援開発向けに生成する。

```
build/compile_commands.json
```

VSCodeやClaude Codeなどが利用可能。

必要に応じてルートへシンボリックリンク：

```
ln -s build/compile_commands.json .
```

---

# 15. VSCode推奨拡張

推奨：

- C/C++
- CMake Tools
- clangd
- Cortex-Debug

clangd利用推奨。

---

# 16. 推奨clangd設定

```
mkdir -p .vscode

cat << 'EOF' > .vscode/settings.json
{
  "clangd.arguments": [
    "--compile-commands-dir=build"
  ]
}
EOF
```

---

# 17. Git初期化

githubからcloneしていた場合は不要。


```
git init

git add .

git commit -m "Initial PicoCalc GB project"
```

---

# 18. 次の作業

次に行う推奨作業：

1. PicoCalc LCD仕様調査
2. PicoCalc keyboard matrix調査
3. SDカード動作確認
4. GBエミュコア選定
5. Peanut-GB / Walnut-CGB評価
6. LCD描画テスト
7. 入力テスト
8. ROMロード実装

---

# 19. 推奨開発フロー

```
変更
 ↓
cmake --build build
 ↓
UF2コピー
 ↓
PicoCalc動作確認
 ↓
git commit
```

---

# 20. 将来的な構成

将来的には以下の構成を目指す。

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

これにより、B-2で構築したGB/GBCコアをA-2へ移植しやすくする。

---

# 21. トラブルシューティング

## 21.1 cmakeでPICO_SDK_PATHエラー

確認：

```
echo $PICO_SDK_PATH
```

再読込：

```
source ~/.bashrc
```

---

## 21.2 gcc-arm-none-eabi が見つからない

確認：

```
arm-none-eabi-gcc --version
```

未導入の場合：

```
sudo apt install gcc-arm-none-eabi
```

---

## 21.3 UF2が認識されない

- BOOTSEL押下確認
- USBケーブル確認
- dmesg確認

---

## 21.4 compile_commands.json が生成されない

cmake実行時に以下を確認：

```
-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

---

# 22. 当面の目標

最初の目標：

- PicoCalc LCD初期化
- Hello World表示
- キー入力確認
- SDカードからROMロード
- GB ROM起動
- 「カエルのために鐘は鳴る」タイトル表示

まずは「音なし・最低限UI」でよい。