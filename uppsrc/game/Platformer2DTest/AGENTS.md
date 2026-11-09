# Platformer2DTest Package Guide

## Scope
- Applies to `uppsrc/game/Platformer2DTest/`.
- Headless unit, format, decompression, simulation, and differential regression tests.

## Responsibilities

1. **Unit Tests**
   - Decompression: Huffman, Carmack, RLEW, LZW round-trip
   - Resource readers: endian-safe, bounds checking
   - Episode variable parsing
   - Map plane loading
   - Renderer primitives (blit, 1bpp, palette)
   - Audio: OPL register writes, PC speaker, mixing
   - Simulation: object updates, collision, physics

2. **Format Tests**
   - Chunk header parsing
   - GAMEMAPS/EGAGRAPH/AUDIO index reading
   - EPISODE.CKx variable definitions
   - Save game format

3. **Differential Regression Tests**
   - Run OmniSpeak demo dumps through same simulation
   - Compare game state, object fields, timing
   - Compare against checked-in dump fixtures
   - Framebuffer pixel comparison

4. **Test Fixtures**
   - Synthetic/BSD-licensed test data only
   - No original game data committed
   - Generators for minimal valid resources

## Key Types

- `TestFixture` - Base class for test data
- `DumpComparator` - Compares Platformer2D dumps to OmniSpeak dumps
- `FramebufferComparator` - Pixel-perfect framebuffer comparison

## Test Structure

```
Platformer2DTest/
├── CoreTests.cpp      # Game state, objects, actions
├── ResourceTests.cpp  # Decompression, chunk loading, maps
├── RenderTests.cpp    # Blitting, palette, fonts
├── AudioTests.cpp     # OPL, mixing, IMF
├── DiffTests.cpp      # Differential vs OmniSpeak dumps
└── Fixtures/          # Synthetic test data
```

## Running Tests

- Build with `bin/build -M 0 -m MSVS26x64 Platformer2DTest` (Debug)
- Run executable for automated test suite
- Headless mode: no window, manual tick advancement
- Exit code: 0 = all passed, non-zero = failures

## Dependencies

- Uses: `Platformer2D`, `Core`, `Resource`, `Render`, `Audio`, `Platform` (headless)
- No GUI dependency in headless mode