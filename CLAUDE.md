# Plowman — Project Rules & Constraints

## Target Hardware
Amiga OCS — PAL 320×256, 5 bitplanes, double-buffered

## Framework
ACE Framework located in `framework/ace/`

## ⚠️ Critical Constraints
- **`framework/ace/` is READ-ONLY** — never modify any files under this path. All custom game logic, viewport configuration, and setup must use the framework APIs as-is from within `src/`
- Use **camelCase** for all variable and pointer names (no Hungarian notation)
  - ✅ `view`, `vPort`, `buffer`, `stateManager`
  - ❌ `g_pView`, `g_pVPort`, `g_pBuffer`, `g_pStateManager`

## Code Style
- Plain camelCase for all variables and pointers
- No `g_p`, `g_`, or any other Hungarian-style prefixes
