# Antigravity Session Handoff: Plowman Audio, Video & Custom Copper Integration

This document serves as a complete handoff for the **Antigravity CLI (`agy`)** to pick up development on the Amiga OCS game **plowman**. It outlines the current state of the codebase, recent fixes, core constraints, hardware specifications, and documentation locations.

---

## 📂 Key Directories & Documentation Reference

- **Workspace Path**: `/Users/mattswart/Source/Amiga/plowman`
- **ACE Framework Docs**: `/Users/mattswart/Source/Amiga/ACE/docs` (Standard guides for views, blitter, palette, audio, OS)
- **Target Hardware Specs**: [docs/a500-specs.md](file:///Users/mattswart/Source/Amiga/plowman/docs/a500-specs.md) (Standard A500 512KB Chip + 512KB Slow RAM profile)
- **ACE Framework Source**: `framework/ace/` (Strictly **READ-ONLY**)

---

## 🛠️ Summary of Refactoring & Features Implemented

### 1. Renamed Hungarian Notation to camelCase
Refactored all main game state pointers to adhere strictly to the clean camelCase style preference:
- `g_pView` ➔ `view`
- `g_pVPort` ➔ `vPort`
- `g_pBuffer` ➔ `buffer`
- `g_pIntroState` ➔ `introState`
- `g_pStateManager` ➔ `stateManager`

### 2. Keyboard & OS State Controls
- **Wired up Key Manager**: Fully integrated `keyCreate()`, `keyProcess()`, and `keyDestroy()` into the main game lifecycle.
- **Edge-Triggered Exit**: Configured the ESC check inside `introLoop()` to use `keyUse(KEY_ESCAPE)` so the key is consumed cleanly on release, triggering `gameExit()`.
- **Global OS Disabling**: Integrated `systemUnuse()` at the end of `genericCreate()` and `systemUse()` at the start of `genericDestroy()` to stop Amiga OS multitasking during active gameplay.

### 3. Integrated ptplayer Music & SFX
- **ProTracker loops**: Set up CIA-B interrupts in PAL mode using `ptplayer` to play the `coal-prelude.mod` file in an infinite loop.
- **Custom WAV-to-SFX converter**: Created a custom Python script at `tools/wav2sfx.py` that processes raw `.wav` inputs, shifting samples to signed 8-bit mono, word-padding, and zero-padding the first word to meet `ptplayer` hardware playback constraints. Integrated directly into the `Makefile` under `sound/sfx/`.
- **Sound Effects timer**: Set up a timer to play `hud_msg.sfx` every 250 frames (exactly 5.0 seconds in PAL mode) on any available channel.

### 4. Raw Background Bitmap Loading
- **splash bitmap**: Configured `introCreate()` to load the raw, headerless 320x239 5-bitplane `bpl/plowman_splash.bpl` (9560 bytes per plane) directly into memory and blit it onto both simple buffer pages (`buffer->pFront` and `buffer->pBack`) to guarantee clean double-buffered display pages.

### 5. Native ACE Copper List Palette Integration
- **Custom Copper Block**: Rather than doing CPU register pokes (which can flash or jitter during system interrupts), we utilize ACE's official copper list API:
  - We allocate a custom `tCopBlock` at wait position `(0, 0)` in the active viewport copper list `view->pCopList`.
  - We write 32 copper MOVE instructions targeting `g_pCustom->color[i]` using `copMove()`.
  - This allows the copper hardware to automatically update the palette colors every frame in the background with zero CPU overhead.
  - The block is cleanly destroyed on exit via `copBlockDestroy()`.

### 6. Instantly Visible Splash & OS State Optimization
- **Startup Reordering**: To eliminate the perceptional 3-4 second black screen while loading heavy audio files, we reordered `introCreate()`. It now loads the palette and raw splash image first, then calls `viewLoad(view)` to display it instantly. Only *then* does it initialize `ptplayer` and load the music and sound effects.
- **OS Batching**: Wrapped `introCreate()` and `introDestroy()` in single, matching `systemUse()` and `systemUnuse()` calls to batch OS requests during dynamic disk/allocation routines, speeding up transitions by ~0.5s.

---

## 🚀 Build & Run Commands

Compile on the host system:
```bash
make clean && make
```

Run in an emulator (FS-UAE, WinUAE, etc.) targeting Amiga 500 / OCS (PAL mode).
