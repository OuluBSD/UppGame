# Platformer2DKdumper - DOS Patch Reference

This package documents the `kdumper` workflow from OmniSpeak for reference purposes.

## Purpose

The `kdumper` tool patches DOS executables to emit runtime state dumps (timecount, game state, object list) for differential testing. It is NOT a runtime component of Platformer2D.

## Files (from OmniSpeak)

| File | Purpose |
|------|---------|
| `kdumper.inc` | Main assembly patch logic |
| `k4e14dmp.asm` | Keen 4 v1.4 patch source |
| `k4e14dmp.pat` | Keen 4 v1.4 binary patch |
| `k5e14dmp.asm` | Keen 5 v1.4 patch source |
| `k5e14dmp.pat` | Keen 5 v1.4 binary patch |
| `k6e14dmp.asm` | Keen 6 v1.4 patch source |
| `k6e14dmp.pat` | Keen 6 v1.4 binary patch |
| `k6e15dmp.asm` | Keen 6 v1.5 patch source |
| `k6e15dmp.pat` | Keen 6 v1.5 binary patch |

## Workflow

1. Obtain original DOS executables (KEEN4E.EXE, KEEN5E.EXE, KEEN6.EXE)
2. Apply corresponding `.pat` file using a DOS patcher
3. Run patched executable in DOSBox
4. Execute demo to generate dump file
5. Use `Platformer2DDumpPrinter` to decode dump
6. Compare with Platformer2D generated dumps

## Patch Format

The patches modify the DOS executable to:
- Hook the game loop
- Write timecount, game state, and object array to a dump file
- Continue normal execution

## Integration with Platformer2D

Generated dumps are used as test fixtures in `Platformer2DTest/DiffTests.cpp` for differential regression testing.

## Note

This package has no build target. The assembly patches are source artifacts for reference only.