# UppGame

A high-performance gaming framework, 2D/3D platformer runtime, launcher, and scriptable adventure engine built natively in C++ using the Ultimate++ (U++) framework.

## Packages

### 1. `GameLib`
Foundational game engine types, geometry abstractions, coordinates, and math utilities.

### 2. `GameEngine`
Higher-level game engine subsystems including:
- Entity-Component-System (ECS) integration
- Animation, Sprites, Texture Atlas, and Tilemap renderers
- 3D Lighting, Shadow mapping, and Scene Graph
- Audio system, streaming, and effects
- Input system with mobile touch & multi-touch abstractions

### 3. `GameLauncher`
Standalone `.gamestate` runner and launcher.

### 4. `Adventure`
Scriptable graphical adventure engine with Python language bindings.

### 5. `game`
2D platformer implementations and toolchains (`Platformer2D`, `Platformer2DTest`, `Platformer2DDumpPrinter`).

---

## Building

Build with `uppbuilder`:

```bash
uppbuilder tests/GameCheck -m CLANG --bazaarcore <path_to_BazaarCore> --uppgeometry <path_to_UppGeometry> -v
```

## CI Matrix
UppGame is continuously verified on all 4 tier-1 platforms:
- Ubuntu Linux with Clang
- Ubuntu Linux with GCC
- macOS Apple Silicon (arm64) with Clang
- Windows x64 with MSVC
