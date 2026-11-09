# Platformer2D/Core Package Guide

## Scope
- Applies to `uppsrc/game/Platformer2D/Core/`.
- Fixed-step simulation, episode selection, game state, object/action system, collision/physics, demo/save handling.

## Responsibilities

1. **Episode Management**
   - Episode registration and selection (Keen 4/5/6 v1.4/v1.5)
   - Episode-specific function tables (setup, presence check, logic)

2. **Game State**
   - Player state (position, score, lives, inventory)
   - Level state (current level, difficulty, map position)
   - Episode-specific variables (wetsuit, security card, sandwich, etc.)
   - Save/load game serialization

3. **Object/Action System**
   - Object list management (100 objects × 38 fields)
   - Action definitions and function pointers (think, collide, draw)
   - State machine for object behaviors

4. **Collision/Physics**
   - Tile-based collision detection
   - Platform physics (moving platforms, conveyor belts)
   - Keen movement physics (gravity, jumping, pogo)

5. **Demo System**
   - Demo recording (input + time events)
   - Demo playback with deterministic simulation
   - Demo file format compatibility

6. **Fixed-Step Clock Contract**
   - 70 Hz simulation tick (matching original PIT)
   - Injected time source for headless testing
   - Frame interpolation for rendering

## Key Types (ported from OmniSpeak)

- `CK_EpisodeDef` - Episode function table
- `CK_GameState` - Complete game state (map pos, levels done, score, episode-specific vars, key gems, current level, lives, difficulty)
- `CK_action` - Action definition (chunkLeft, chunkRight, type, protectAnimation, stickToGround, timer, velX, velY, think, collide, draw, next)
- `CK_object` - Object state (38 16-bit fields matching dumpprinter schema)
- `CK_Difficulty` - Easy/Normal/Hard
- `CK_LevelState` - Playing/ResetGame/LoadedGame/...

## Interfaces

- `ISimulationClock` - Injected time source (GetTickCount, SetTickCount, WaitTick)
- `IRandomGenerator` - Injected RNG for deterministic replay
- `IInputProvider` - Injected input for demo playback

## Dependencies

- Uses: `Platformer2D` (main package), `Platformer2D/Resource`
- Used by: `Platformer2D/Platform`, `Platformer2DTest`

## Binary Compatibility

- Game state dump schema must match OmniSpeak `dumpprinter` exactly:
  - Timecount (uint32)
  - GameState (variable size per episode)
  - 100 Objects × 38 uint16 fields each
- All numeric types use explicit fixed-width types (uint16_t, int32_t, etc.)
- Endian-safe serialization for cross-platform save compatibility