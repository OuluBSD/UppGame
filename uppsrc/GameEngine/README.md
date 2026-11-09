# GameEngine Package

The GameEngine package provides a comprehensive game development framework that builds upon the U++ ecosystem and integrates with the Eon system for real-time, parallel processing capabilities.

## Features

- **Game Window Management**: Cross-platform windowing with game loop integration
- **Input System**: Unified keyboard, mouse, and gamepad input handling
- **Audio System**: 3D audio with sound management capabilities  
- **Physics System**: Rigid body dynamics and collision detection
- **UI System**: Heads-up display and menu rendering
- **Virtual File System**: Cross-platform asset management
- **Animation System**: Transform and skeletal animation support
- **Asset Management**: Resource loading and memory budget tracking
- **Scene Management**: Scene switching and state management
- **2D/3D Rendering**: Integrated rendering pipelines
- **Geometry Integration**: Vector, matrix, and 3D math operations

## Architecture

The GameEngine follows a modular architecture where each system operates independently but integrates seamlessly:

- Core systems: Game, GameWindow, AssetManager, Scene
- Input/Output: InputSystem, AudioSystem, VFS
- Rendering: Rendering pipeline, UI system
- Simulation: Physics system, Animation system

## Dependencies

- Core U++ packages: Core, Draw, CtrlCore, Geometry
- API packages: api/Screen, api/Graphics, api/Hal, api/Audio, api/Physics
- Eon system: For ECS and parallel processing capabilities

## Usage

```cpp
#include <GameEngine/GameEngine.h>

int main() {
    Game game;
    game.Run();
    return 0;
}
```

For detailed API documentation and usage examples, see GameEngine.md.

## Development Status

- [x] Basic framework and windowing
- [x] Input system integration
- [x] Audio system implementation
- [x] Physics system integration
- [x] UI/HUD rendering capabilities
- [x] Cross-platform VFS
- [x] Animation system
- [x] ECS integration (now implemented)
- [ ] Advanced rendering features
- [ ] Networking support
- [ ] Scripting integration

The GameEngine now includes ECS integration, following data-oriented design principles for optimal performance. You can use the GameEcsIntegration class to work with entities and components in your games.