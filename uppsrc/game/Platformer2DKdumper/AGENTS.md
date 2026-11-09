# Platformer2DKdumper Package Guide

## Scope
- Applies to `uppsrc/game/Platformer2DKdumper/`.
- Documents the `kdumper` workflow as a reference; assembly patches are source artifacts, not portable runtime code.

## Responsibilities

1. **Documentation**
   - DOS patch logic for runtime state extraction
   - Patch files for each episode version (Keen 4/5/6 v1.4/v1.5)
   - Assembly source for reference

2. **Workflow Documentation**
   - How to apply patches to DOS executables
   - How to run patched executables to generate dumps
   - How to convert dumps to fixtures

3. **Reference Implementation**
   - U++ tool to validate/apply patches (optional)
   - Not a runtime component

## Key Files (from OmniSpeak)

- `kdumper.inc` - Main patch logic
- `k4e14dmp.asm/pat` - Keen 4 v1.4
- `k5e14dmp.asm/pat` - Keen 5 v1.4
- `k6e14dmp.asm/pat` - Keen 6 v1.4
- `k6e15dmp.asm/pat` - Keen 6 v1.5

## Notes

- This package is for documentation/reference only
- The actual DOS patching requires a DOS environment
- Generated dumps are used as test fixtures for differential testing
- Assembly patches are NOT compiled as part of the U++ build

## Dependencies

- Uses: `Platformer2D` (for dump format definitions)
- No build targets