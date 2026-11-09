# Umbrella Game Project

A 2D platformer game built with Ultimate++ framework, featuring a map editor and automated testing infrastructure.

## Project Overview

Umbrella is a platformer game where the player navigates levels using an umbrella for combat and gliding. The project includes:

- **Game Engine**: Fixed timestep physics, entity system, collision detection
- **Map Editor**: Full-featured level editor with layers, tools, and reference image support
- **Automation Framework**: Python-based testing using ByteVM for both editor and gameplay
- **AI Planning**: A*-based automated playthrough system

## Build Instructions

**Important**: Always use `script/build.py` to build, NOT `theide` directly:

```bash
script/build.py -mc 1 -j 12 Umbrella
```

## Running the Game

```bash
# Launch main menu
bin/Umbrella

# Start with specific level
bin/Umbrella --game world1 stage1

# Open map editor
bin/Umbrella --editor

# Open specific level in editor
bin/Umbrella --editor share/mods/umbrella/levels/world1-stage1.json

# Run automated tests
bin/Umbrella --test tests/gameplay/test_player_movement.py
```

## Project Structure

```
game/Umbrella/
├── main.cpp                    # Entry point with command-line routing
├── GameScreen.h/cpp            # Main game loop and state machine
├── Player.h/cpp                # Player physics and mechanics
├── Enemy.h/cpp                 # Enemy base class
├── EnemyPatroller.h/cpp        # Simple walker AI
├── EnemyJumper.h/cpp          # Jumping AI
├── EnemyShooter.h/cpp         # Stationary shooter with projectiles
├── Treat.h/cpp                 # Collectible rewards
├── Droplet.h/cpp              # Water drop collectibles
├── MapEditor.h/cpp            # Level editor GUI
├── MapSerializer.h/cpp        # JSON level loading/saving
├── LayerManager.h/cpp         # Layer system for tiles
├── GameScriptBridge.h/cpp     # Python automation API (planned)
├── ActionPlanner.h/cpp        # AI planning system (planned)
├── tests/                     # Automated test scripts
│   ├── editor/                # Editor automation tests
│   └── gameplay/              # Gameplay automation tests
└── plan/                      # Planning documentation
    ├── automation/            # Automation tracks (Track 1 & 2)
    ├── implementation/        # Core game implementation
    └── cookie.txt            # Progress tracking
```

## Current Development Status

**Completed Features:**
- Core game engine with fixed timestep (60 FPS)
- Player mechanics: movement, jumping, umbrella combat/gliding, enemy capture
- Three enemy types: Patroller, Jumper, Shooter
- Level transitions with hover/scroll/drop animations
- Map editor with DockWindow panels
- Brush/Eraser/Fill tools with multiple sizes
- Layer system with opacity and visibility control
- Reference image support for level tracing
- JSON level loading and saving
- 36 playable levels across multiple worlds

**Current Focus: Automation Foundation**

See `plan/automation/` for detailed roadmap of automation and AI planning work.

## Automation Framework

The automation system uses ByteVM (Python interpreter) to script both editor operations and gameplay. Key features:

**Editor Automation:**
- GUI element discovery via hierarchical paths
- Click, type, and verify operations
- Layer manipulation and tool testing
- Batch level operations

**Gameplay Automation:**
- Direct game state manipulation
- Input injection without keyboard events
- Frame-by-frame replay system
- Entity state assertions
- Deterministic physics via random seed control

**Example Test Script:**
```python
# Load level
load_level("world1-stage1.json")

# Set player position
set_player_position(100, 200)

# Simulate input
simulate_input(K_RIGHT, True)   # Press right
wait_frames(30)                 # Wait 0.5 seconds
simulate_input(K_RIGHT, False)  # Release

# Assert results
player_pos = get_player_position()
assert player_pos[0] > 100, "Player should have moved right"
```

## AI Planning System

The AI planner enables automated playthrough using A* pathfinding and action primitives:

- **Pathfinding**: Navigate level geometry using A*
- **Action Primitives**: move, jump, attack, glide, capture_enemy
- **Goal System**: reach_position, kill_all_enemies, collect_treats
- **Plan Execution**: Execute action sequences with real-time corrections

This enables:
- Automated QA testing of all levels
- Difficulty balancing via success metrics
- Level validation (reachable goals, no dead ends)
- Player behavior analysis

## Key Gameplay Mechanics

**Player:**
- Move speed: 140 px/s
- Jump velocity: 280 px/s
- Gravity: -490 px/s²
- Parasol states: IDLE, ATTACKING (0.25s), GLIDING (slow fall)
- Can capture up to 3 enemies (carry weight 3.0)
- Throw captured enemies horizontally at 350 px/s

**Enemies:**
- **Patroller**: Walks at 70 px/s, reverses at walls/ledges
- **Jumper**: Walks at 80 px/s, jumps every 1.5-2.5s (random)
- **Shooter**: Stationary, detects player within 210px, shoots every 2s

**Physics:**
- Fixed timestep: 1/60 second (0.0167s)
- Grid size: 14 pixels per tile
- Collision resolution in 3.5px steps
- Coyote time: 0.1s after leaving ground
- Jump buffer: 0.12s input tolerance

## Map Editor Features

**Tools:**
- Brush (B): Paint tiles with 1x1/2x2/3x3/5x5 sizes
- Eraser (E): Remove tiles (or right-click for quick erase)
- Fill (Shift+F): Flood fill contiguous regions

**Layers:**
- Terrain: Collision tiles (walls, blocks)
- Background: Visual tiles
- Entities: Enemy/droplet spawn points
- Annotations: Reference images

**Workflows:**
- Load reference images (Ctrl+R) for tracing
- Adjust opacity (R to toggle, slider for fine control)
- Pan reference independently (Ctrl+Middle-drag)
- Recent files menu for quick access
- Editable map size with apply button

## Testing

**Unit Tests:**
```bash
bin/GameScreenTest
```

**Automation Tests:**
```bash
# Editor tests
bin/Umbrella --test tests/editor/test_brush_tool.py
bin/Umbrella --test tests/editor/test_layer_management.py

# Gameplay tests
bin/Umbrella --test tests/gameplay/test_player_movement.py
bin/Umbrella --test tests/gameplay/test_enemy_ai.py
bin/Umbrella --test tests/gameplay/test_level_completion.py
```

## Documentation

- **AGENTS.md**: General AI agent conventions and policies
- **CLAUDE.md**: Claude-specific development guidelines
- **plan/**: Detailed planning documents for all tracks
- **plan/cookie.txt**: Current progress tracking

## Contributing

Follow the conventions in AGENTS.md and CODESTYLE.md. Always:
1. Read relevant task/*.md files before starting work
2. Use `script/build.py` for building
3. Update cookie.txt with progress
4. Test changes with automated tests where possible
5. Commit with clear messages describing the "why"

## License

See LICENSE file for details.
