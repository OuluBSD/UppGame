# Platformer2DDumpPrinter Package Guide

## Scope
- Applies to `uppsrc/game/Platformer2DDumpPrinter/`.
- U++ CLI port of OmniSpeak `dumpprinter` tool.

## Responsibilities

1. **Dump File Parsing**
   - Binary dump format: timecount + gameState + 100 objects × 38 fields
   - Episode-specific game state parsing (Keen 4/5/6)
   - Little-endian reading with host conversion

2. **Output Formatting**
   - Hex output matching OmniSpeak dumpprinter exactly
   - Timecount: `%08X`
   - GameState fields: space-separated hex
   - Objects: `Obj %02X:` prefix + 38 hex fields

3. **CLI Interface**
   - `Platformer2DDumpPrinter <dumpfile> <episode>`
   - Episode: 4, 5, or 6

## Binary Compatibility

- Output must be byte-for-byte identical to OmniSpeak dumpprinter
- All fields printed as unsigned 16-bit hex
- Episode-specific unions handled correctly

## Dependencies

- Uses: `Platformer2D`, `Core` (for CK_GameState definition)
- No GUI, no platform dependencies