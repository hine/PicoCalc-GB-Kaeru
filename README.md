# PicoCalc-GB-Kaeru

Game Boy emulator for [ClockworkPi PicoCalc](https://www.clockworkpi.com/picocalc), running on the RP2350A.

The name comes from the first game tested: *For the Frog the Bell Tolls* (カエルのために鐘は鳴る).

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
| WASD / ↑↓←→ | D-Pad |
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

To write a ROM to Flash, place `rom.gb` on the SD card's root directory and power on. The firmware detects it, copies it to Flash, and removes the file from the SD card.

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
