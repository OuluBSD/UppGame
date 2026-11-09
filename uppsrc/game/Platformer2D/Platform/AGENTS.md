# Platformer2D/Platform Package Guide

## Scope
- Applies to `uppsrc/game/Platformer2D/Platform/`.
- U++ window/input/timer/audio adapters; no platform code in core/resource packages.

## Responsibilities

1. **Window Management (CtrlLib)**
   - TopWindow with custom title, icon
   - Fullscreen/windowed toggle
   - Aspect ratio correction (4:3 for 320x200)
   - Integer scaling modes (1x, 2x, 3x...)
   - Overscan border emulation
   - VSync control

2. **Input Mapping (CtrlLib → IN_ControlFrame)**
   - Keyboard: configurable key mappings (jump, pogo, fire, directions)
   - Joystick/Gamepad: SDL-style mapping via CtrlLib
   - Mouse: menu cursor support
   - Text input: for name entry, etc.

3. **Timer (70 Hz fixed step)**
   - U++ TimerCallback at 70 Hz
   - Accumulator for fixed-step simulation
   - Frame interpolation for rendering
   - Headless mode: manual tick advancement

4. **Audio Output (U++ Sound)**
   - Sound::SetCallback for audio mixing
   - Stereo 44.1kHz output
   - Null callback for headless testing
   - Volume control

5. **Startup/Shutdown Sequence**
   - Initialize: Platform → Resource → Core → Render → Audio
   - Run: Main loop (fixed-step simulation + render)
   - Shutdown: Reverse order

6. **Command Line / Config**
   - Parse /EPISODE, /FULLSCREEN, /INTEGER, /NOBORDER, etc.
   - Load OMNISPK.CFG
   - Episode auto-detection

## Key Types

- `Platformer2DWindow` - Main window class
- `InputMapper` - CtrlLib events → IN_ControlFrame
- `FixedStepTimer` - 70 Hz simulation clock
- `AudioOutput` - U++ Sound callback adapter

## Headless Mode

- All subsystems initialize without window
- Timer advances manually via `Tick()`
- Audio uses null callback
- Render captures framebuffer for comparison

## Dependencies

- Uses: `Platformer2D`, `Core`, `Resource`, `Render`, `Audio`
- Used by: `Platformer2DTest` (for headless), main entry point

## U++ Integration

- Main entry: `CONSOLE_APP_MAIN` or `GUI_APP_MAIN`
- Uses `CtrlLib` for window/input
- Uses `Sound` for audio output
- Uses `Draw` for presentation (IndexedSurface → Image)