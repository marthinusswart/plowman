# Antigravity Session Handoff: Plowman Refactoring & PAL Setup

This document serves as a complete handoff for the **Antigravity CLI (`agy`)** to pick up development on the Amiga OCS game **plowman**. It outlines the current state of the codebase, recent fixes, core constraints, and recommended next steps.

---

## 📌 Project Context & Rules
* **Project Name:** plowman
* **Target Hardware:** Amiga OCS (PAL 320x256, 5 bitplanes, double-buffered)
* **Framework:** **ACE Framework** located in `framework/ace/`.
* **⚠️ Critical Constraint:** **The framework is strictly read-only.** Never touch any files under `framework/ace/`. All custom game logic, viewport configuration, and setups must use the framework APIs as-is from within `src/`.
* **Code Style:** Do **not** use Hungarian notation (e.g., no `g_p` prefixes). Use plain `camelCase` for variable and pointer names (e.g., `view`, `vPort`, `stateManager`).

---

## 🛠️ Summary of Fixes Implemented in this Session

### 1. Renamed Hungarian Notation to camelCase
Refactored all main game state pointers to adhere strictly to the clean camelCase style preference:
* `g_pView` ➔ `view`
* `g_pVPort` ➔ `vPort`
* `g_pBuffer` ➔ `buffer`
* `g_pIntroState` ➔ `introState` (now handled within the state manager context)
* `g_pStateManager` ➔ `stateManager`

Files updated:
* [src/game.h](file:///Users/mattswart/Source/Amiga/plowman/src/game.h)
* [src/game.c](file:///Users/mattswart/Source/Amiga/plowman/src/game.c)
* [src/gamelogic/intro/intro.c](file:///Users/mattswart/Source/Amiga/plowman/src/gamelogic/intro/intro.c)

### 2. Wired up the ACE Key Manager
* **Issue:** The original loop was missing the initialization and processing of the keyboard, so keypresses were never detected.
* **Fix:** Integrated `keyCreate()` into `genericCreate()`, `keyProcess()` at the top of `genericProcess()`, and `keyDestroy()` in `genericDestroy()`.

### 3. Edge-Triggered Exit (ESC Key)
* **Issue:** The intro loop was previously using a level-triggered check which could cause double-events or irregular behavior.
* **Fix:** Changed keyboard detection in `introLoop()` to use `keyUse(KEY_ESCAPE)` (edge-triggered, marking the key as consumed) instead of `keyCheck` to trigger a clean exit via `gameExit()`.

### 4. Enforced PAL Mode without Framework Modifications
* **Issue:** The framework automatically detects PAL/NTSC based on system VBlank frequency. To force PAL dimensions globally for a PAL-specific game without touching the read-only framework:
* **Fix:** Supplied explicit PAL tags when creating the view and viewport in `src/game.c` utilizing constants from `ace/generic/screen.h`:
  ```c
  viewCreate(
      TAG_VIEW_WINDOW_WIDTH, SCREEN_PAL_WIDTH,
      TAG_VIEW_WINDOW_HEIGHT, SCREEN_PAL_HEIGHT,
      TAG_VIEW_WINDOW_START_Y, SCREEN_PAL_YOFFSET,
      TAG_DONE
  );
  ```

### 5. Fixed Double-Free Bug in stateManager Cleanup
* **Issue:** `genericDestroy()` was explicitly calling `stateDestroy(introState)` followed by `stateManagerDestroy(stateManager)`. Because the state manager internally manages and frees states that have been pushed onto its stack, this led to a double-free crash.
* **Fix:** Removed the explicit `stateDestroy(introState)` call, letting `stateManagerDestroy(stateManager)` handle the cleanup cleanly.

### 6. Integrated Bartman GCC Debug Support
* Retained the user's custom additions:
  * Included `"support/gcc8_c_support.h"` to enable advanced debugging tools.
  * Added `KPrintF("Starting game\n")` for serial-port logging.

---

## 📂 Current File Directory Status

### [game.c](file:///Users/mattswart/Source/Amiga/plowman/src/game.c)
* Configures view, viewport, double-buffered simple buffer manager, key manager, and pushes the initial `introState` onto the state manager.
* Main loop processes inputs, updates state machine, and swaps double-buffers cleanly.

### [game.h](file:///Users/mattswart/Source/Amiga/plowman/src/game.h)
* Declares core globals as `extern` using clean `camelCase` structure.

### [intro.c](file:///Users/mattswart/Source/Amiga/plowman/src/gamelogic/intro/intro.c)
* Basic state container for the introduction phase of the game.
* Currently only monitors the keyboard for `KEY_ESCAPE` to call `gameExit()`.

---

## 🚀 Suggested Next Steps for `agy` CLI
Here are tasks you can prompt `agy` to tackle next:
1. **Set Up Palette & Background Color:**
   * *"Create and load a standard color palette for our vPort and clear the double-buffer to a dark background color."*
2. **Display Title/Intro Graphics:**
   * *"Load a 5-plane PAL bitmap for our title screen and blit it to the simple buffer inside `introCreate()` or `introLoop()`."*
3. **Add Text Rendering:**
   * *"Set up an ACE text font and write 'Press ESC to Exit' centered at the bottom of the intro viewport."*
