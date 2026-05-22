# PicoCalc-GB-Kaeru

RP2350A + ClockworkPi PicoCalc 上で Game Boy エミュレータを動作させるプロジェクト。
初期ターゲットは「カエルのために鐘は鳴る」（DMG Game Boy）。

---

## ドキュメント

| ファイル | 内容 |
|---|---|
| **README.md**（本ファイル） | プロジェクト概要・ファイル構成の案内 |
| [Spec.md](Spec.md) | プロジェクト仕様・方針・マイルストーン定義 |
| [PROGRESS.md](PROGRESS.md) | 開発進捗・チェックリスト・決定事項ログ。**作業再開時はまずここを確認** |
| [HardwareSpec.md](HardwareSpec.md) | PicoCalc のハードウェア仕様（LCD・キーボード・SD・音声のピン配置・制御方法） |
| [DevelopmentEnvironment.md](DevelopmentEnvironment.md) | 開発環境構築手順（Pico SDK・pyenv・ビルド方法） |

---

## ハードウェア

| 項目 | 内容 |
|---|---|
| 本体 | ClockworkPi PicoCalc |
| SoC | Raspberry Pi RP2350A |
| CPU | Dual-core Arm Cortex-M33（最大 150MHz） |
| SRAM | 520KB |
| Flash | 16MB |

---

## ソフトウェア構成

```
PicoCalc-GB-Kaeru/
├── src/
│   ├── main.c          エントリポイント
│   ├── platform/       PicoCalc ハードウェア初期化（LCD・KB・SD・音声）
│   ├── video/          LCD 描画・フレームバッファ転送
│   ├── input/          キーボード入力・GB キーマッピング
│   ├── audio/          音声出力（PWM / I2S）
│   ├── storage/        ROM・セーブデータ読み書き（FatFS）
│   └── system/         セーブステート・設定
├── emu/
│   └── gb/             GB/GBC エミュレーションコア（Peanut-GB 予定）
├── roms/               ROM ファイル置き場（Git 管理外）
├── saves/              セーブデータ置き場（Git 管理外）
├── tools/              開発補助スクリプト等
└── build/              ビルド生成物（Git 管理外）
```

エミュレーションコア（`emu/`）と PicoCalc 依存コード（`platform/`・`video/` 等）を分離し、
将来的な ESP32-S3 端末（A-2）への移植を見据えた構造とする。

---

## ビルド方法

```sh
cmake -S . -B build -G Ninja -DPICO_BOARD=pico2 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

生成物: `build/picocalc_gb_kaeru.uf2`

PicoCalc への書き込みは BOOTSEL モードで USB 接続後、UF2 をコピーする。

```sh
cp build/picocalc_gb_kaeru.uf2 /media/$USER/RPI-RP2/
```

詳細は [DevelopmentEnvironment.md](DevelopmentEnvironment.md) を参照。

---

## 現在の状態

`PROGRESS.md` を参照。現在は **Milestone 1: PicoCalc 基本I/O** の実装準備中。

---

## ROM・セーブデータの取り扱い

ROM ファイル・セーブデータはリポジトリに含めない（`.gitignore` で除外）。
ユーザーが所有・吸い出し済みの ROM のみ使用する。
