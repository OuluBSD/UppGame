# Platformer2D Package Guide

## Scope
- Applies to `uppsrc/game/Platformer2D/` and its subpackages.
- This is a U++ port of OmniSpeak (Commander Keen 4/5/6 reimplementation).

## Package Layout

| Subpackage | Purpose |
|------------|---------|
| `Platformer2D/` | Main runtime package (`Platformer2D.upp`, `Platformer2D.h`) |
| `Core/` | Fixed-step simulation, episode selection, game state, object/action system, collision/physics, demo/save handling |
| `Resource/` | Filesystem paths, episode variables, endian-safe readers, resource archives, decompression, cache/purge policy, maps, graphics, fonts, audio chunks |
| `Render/` | Indexed EGA surface model, palette conversion, sprite/tile/text drawing, scaling, border, presentation-independent framebuffer |
| `Audio/` | OPL emulation, PC speaker synthesis, mixing, timing, U++ audio output adapter |
| `Platform/` | U++ window/input/timer/audio adapters; no platform code in core/resource packages |

## Key Design Principles

1. **Headless Core**: Game simulation, resource loading, and rendering primitives are platform-agnostic. The interactive frontend is a presentation/input/audio adapter over the same services.

2. **Indexed Framebuffer**: Preserve 8-bit indexed EGA surface model throughout; direct RGB rendering would make palette and pixel regression harder. U++ Draw presentation reads from this framebuffer.

3. **Deterministic Behavior**: All timing, RNG, and input are injected interfaces so headless tests do not depend on a window or wall clock.

4. **Binary Layout Preservation**: Use explicit fixed-width types and dedicated serialization; do not rely on C++ struct packing or host endianness.

5. **External Data Only**: Original game data (GAMEMAPS.CK?, EGAGRAPH.CK?, AUDIO.CK?) are runtime inputs only. Add `.gitignore` patterns before testing with local proprietary files.

6. **Differential Gates**: Refactor one subsystem at a time with dump fixtures as the compatibility oracle. Use `dumpprinter` schema for game state + 100 objects × 38 fields.

## Build Requirements

- Mainconfig must include `GUI` flag for CtrlLib window support.
- Depends on: `Core`, `Draw`, `CtrlLib`, `CtrlCore`, `Geometry`, `Sound` (for audio output).
- Build via `bin/build` with selected mainconfig; inspect actual build log.

## Subpackage Structure

Each subpackage follows U++ conventions:
- `SubPackageName.upp` lists `AGENTS.md` first in `file` section
- Main header `SubPackageName.h` aggregates all sub-headers
- Source files include only the main header first (`#include "Platformer2D.h"`)

## Extension Points

- Episode registration: `CK_EpisodeDef` structure with function pointers for setup, presence check, and episode-specific logic.
- Video/Audio/Input backends: `VL_Backend`, `SD_Backend`, `IN_Backend` interfaces adapted to U++ equivalents.
- Resource decompression: Huffman, Carmack, RLEW, LZW decoders as independent services.

## Testing

- `Platformer2DTest/`: Headless unit, format, decompression, simulation, and differential regression tests.
- Compare against OmniSpeak dump fixtures for Keen 4/5/6 v1.4/v1.5.
- Visual correctness uses framebuffer readback/capture, not only "no crash".

## Idea Notes

- I-001: Evaluate whether BSD data should use a compact archive or remain unpacked after importer produces representative samples.
- I-002: Evaluate a replay format that stores input/time events rather than raw mutable object dumps.
- I-003: Evaluate whether later Eon integration is useful only after the headless simulation contract is stable.