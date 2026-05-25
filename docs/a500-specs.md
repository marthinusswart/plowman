# Target Hardware Specs: Amiga 500 Emulation

Under the **Bartman Abyss cross-compiler toolchain** (via the standard "Amiga Assembly" VSCode extension), the default **A500 (Amiga 500)** emulation profile configures the emulator to replicate the most common retail and developer-target hardware setup of the original Amiga 500.

Here are the specifications of that target configuration:

---

## 1. Processing Unit (CPU)
- **Processor**: Motorola **MC68000** (16/32-bit CISC).
- **Clock Speed**: **7.09 MHz** (PAL mode is the standard developer target in Europe/Australia and is used by ACE) or **7.16 MHz** (NTSC).
- **Coprocessors**: Custom Agnus (Blitter & Copper) and Paula (Audio & DMA).

---

## 2. Memory (RAM) Layout
The emulator defaults to the classic "1MB Amiga 500" setup (A500 with a standard trapdoor memory expansion):
- **Chip RAM (512 KB)**: High-speed, graphics- and audio-accessible Chip memory located from `$000000` to `$07FFFF`. All bitplanes, copper lists, blitter sources/destinations, and ProTracker audio samples **must** reside here.
- **Slow/Trapdoor RAM (512 KB)**: Pseudo-fast expansion RAM located at `$C00000` to `$C7FFFF`. This memory is used for program code, variables, and stack, freeing up precious Chip RAM for graphics and audio.
- **Total RAM**: 1 MB.

---

## 3. Custom Chipset (OCS — Original Chip Set)

### Agnus (Fat Agnus)
- **Blitter**: Dedicated DMA hardware for super-fast block transfers, fills, and mask copying.
- **Copper**: A programmable graphics coprocessor synchronized with the display beam, allowing on-the-fly register changes (such as our custom palette block) without CPU overhead.

### Denise
- Handles display output, bitplanes, and hardware sprites.
- Contains **32 color registers** (12-bit RGB, mapping to 4,096 possible colors in standard `0x0RGB` format).

### Paula
- **Audio**: 4 hardware PCM 8-bit audio channels (2 panned left, 2 right) with independent volume and sample-rate/period controls, which the `ptplayer` manager runs.
- Handles disk drive DMA, system timers, and joystick/mouse inputs.

---

## 4. Video & Display Defaults (PAL)
- **Resolution**: **320 × 256** pixels (non-interlaced, Lores) is the active canvas size.
- **Colors**: **5 bitplanes** (32 simultaneous colors) is the standard viewport depth.
- **Refresh Rate**: **50 Hz** (yielding exactly 50 frames per second, which means our sfx timer of 250 frames runs every 5.0 seconds).

---

## 5. Operating System (ROM)
- **Kickstart**: **Kickstart 1.3** ROM (256 KB) is the target OS, providing the standard boot ROM and system library bases (`SysBase`, `DOSBase`, `GfxBase`).
