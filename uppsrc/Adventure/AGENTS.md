# Adventure Package - Python Bindings Guide

**Location**: `/common/active/sblo/Dev/ai-upp/uppsrc/Adventure/`

---

## Overview

This package provides Python bindings for the Adventure game engine, allowing Python scripts to control game logic, camera, UI, and rendering.

**Implementation Status**: ✅ **COMPLETE** - All 52 functions implemented (100%)

---

## Files

### Core Files
- `Adventure.h` / `Adventure.cpp` - Main package header
- `Program.h` / `Program.cpp` - Game engine core
- `AdventureBindings.h` / `AdventureBindings.cpp` - Python binding infrastructure
- `test_bindings.py` - Test script for bindings

### Engine Components
- `Camera.cpp` - Camera control system
- `Ui.cpp` - UI and dialog system
- `Script.cpp` - Script execution engine
- `Game.cpp` - Game logic
- `Room.cpp` / `Object.cpp` / `Actor.cpp` - Game entities
- `Gfx.cpp` / `ProgramDraw.cpp` - Graphics rendering
- `Dialog.cpp` - Dialog system
- `Physics.cpp` - Physics engine

### ESC Scripts (Test Files)
- `Game.esc` - Main game script
- `Demo.esc` - Demo script
- `C8_Intro.esc`, `C8_Part1.esc`, `C8_Part2.esc` - PICO-8 port scripts
- `CarverTest.esc` - Minimal test script

---

## Complete Function Reference

### Core Functions - Room & Camera (12 functions)

| Function | Parameters | Description |
|----------|------------|-------------|
| `change_room(room, fade)` | room: EscValue, fade: int | Switch to different room |
| `camera_follow(actor)` | actor: EscValue | Make camera follow actor |
| `camera_at(position)` | position: int or EscValue | Set camera position |
| `camera_pan_to(target)` | target: EscValue | Pan camera smoothly |
| `put_at(obj, x, y, room)` | obj: EscValue, x/y: int, room: EscValue | Position object |
| `walk_to(obj, x, y)` | obj: EscValue, x/y: int | Move object to position |
| `do_anim(obj, anim_name)` | obj: EscValue, anim_name: String | Play animation |
| `get_room()` | - | Get current room |
| `get_actor()` | - | Get current actor |
| `is_in_room(obj, room)` | obj: EscValue, room: EscValue | Check if object in room |
| `get_distance(obj1, obj2)` | obj1, obj2: EscValue | Distance between objects |
| `face_direction(obj, dir)` | obj: EscValue, dir: String | Set object facing |

### UI Functions - Dialog & Text (10 functions)

| Function | Parameters | Description |
|----------|------------|-------------|
| `say_line(text)` or `say_line(actor, text, wait, duration)` | text: String | Display dialog |
| `print_line(text, x, y, color, shadow, centered, width, outline)` | Various | Print text at position |
| `dialog_set(options_array)` | options: String[] | Set dialog options |
| `dialog_start(color, highlight_color)` | color, highlight: int | Start dialog selection |
| `dialog_clear()` | - | Clear dialog options |
| `wait_for_camera()` | - | Wait for camera to finish |
| `fades(fade_type, duration)` | fade_type, duration: int | Screen fade effect |
| `clear_dialog()` | - | Clear active dialog |
| `say_get()` | - | Get last said line |
| `stop_talking()` | - | Stop current dialog |

### Game Logic Functions - Scripts, Inventory, Objects (15 functions)

| Function | Parameters | Description |
|----------|------------|-------------|
| `break_time(frames)` | frames: int (default 1) | Wait N frames |
| `cutscene(type, setup_fn, cleanup_fn)` | type: int, fn: Python function | Start cutscene |
| `pickup_obj(obj, actor)` | obj: EscValue, actor: EscValue | Add to inventory |
| `start_script(script_fn, background)` | script_fn: function, background: bool | Start background script |
| `stop_script(script_fn)` | script_fn: function | Stop script |
| `is_script_running(script_fn)` | script_fn: function | Check script status |
| `verb_set(verb_index)` | verb_index: int | Set active verb |
| `verb_get()` | - | Get active verb |
| `inventory_get()` | - | Get inventory list |
| `has_obj(obj)` | obj: EscValue | Check if has object |
| `use_obj(obj, target)` | obj, target: EscValue | Use object on target |
| `set_selected_actor(actor)` | actor: EscValue | Change controlled actor |
| `get_selected_actor()` | - | Get controlled actor |
| `open_door(door, paired_door)` | door: EscValue, paired: optional | Open a door |
| `close_door(door, paired_door)` | door: EscValue, paired: optional | Close a door |

### Drawing Functions (9 functions)

| Function | Parameters | Description |
|----------|------------|-------------|
| `set_trans_col(color, enable)` | color: int, enable: bool | Set transparent color |
| `map_draw(src_x, src_y, dest_x, dest_y, width, height, flags)` | Various | Draw map tiles |
| `spr(sprite, x, y, w, h, flip_x, flip_y)` | sprite: int, x/y: int | Draw sprite |
| `sspr(sx, sy, sw, sh, x, y, dw, dh, flip_x, flip_y)` | Various | Draw sprite subregion |
| `rect(x, y, w, h, color)` | x/y/w/h/color: int | Draw rectangle |
| `circle(x, y, r, color)` | x/y/r/color: int | Draw circle |
| `line(x1, y1, x2, y2, color)` | x1/y1/x2/y2/color: int | Draw line |
| `pal(from, to)` | from, to: int | Set palette mapping |
| `rectfill(x1, y1, x2, y2, color)` | x1/y1/x2/y2/color: int | Draw filled rectangle |

### Audio Functions (2 functions)

| Function | Parameters | Description |
|----------|------------|-------------|
| `sfx(sound_id)` | sound_id: int | Play sound effect |
| `music(track_id)` | track_id: int | Play music track |

### Object Property Access (4 functions)

| Function | Parameters | Description |
|----------|------------|-------------|
| `obj_get_prop(obj, property)` | obj: EscValue, property: String | Get object property |
| `obj_set_prop(obj, property, value)` | obj: EscValue, property: String, value: any | Set object property |
| `obj_get_x(obj)` | obj: EscValue | Get object X position |
| `obj_set_x(obj, x)` | obj: EscValue, x: int | Set object X position |

---

## Python Binding Pattern

### Function Signature

All binding functions follow this pattern:

```cpp
PyValue function_name(const Vector<PyValue>& args, void* user_data)
{
    // 1. Get Program instance from user_data
    Program* prog = (Program*)user_data;

    // 2. Validate arguments
    if(args.GetCount() < required_args) {
        LOG("Error: function_name requires N arguments"); return PyValue();
    }

    // 3. Extract arguments
    String text = args[0].GetStr().ToString();
    int x = args[1].AsInt();
    EscValue obj = PyToEscValue(args[2]);

    // 4. Call C++ implementation
    prog->SomeFunction(text, x, obj);

    // 5. Return result (or None)
    return PyValue::None();
}
```

### Registration

Bindings are registered in `AdventureBindings::RegisterAll()`:

```cpp
void AdventureBindings::RegisterAll(PyVM& vm, Program& prog)
{
    PyValue globals = vm.GetGlobals();
    
    // Register each function with user_data pointing to Program
    globals.SetItem(PyValue("function_name"), 
                    PyValue::Function("function_name", function_name, &prog));
}
```

### Type Conversion

#### Python → C++
```cpp
// String
String text = args[0].GetStr();

// Integer
int frames = args[0].AsInt();

// Float
double value = args[0].AsDouble();

// Boolean
bool enable = args[0].IsTrue();

// EscValue (game object reference)
EscValue obj = PyToEscValue(args[0]);
```

#### C++ → Python
```cpp
// None
return PyValue::None();

// Integer
return PyValue(42);

// Float
return PyValue(3.14);

// String
return PyValue("hello");

// Boolean
return PyValue::True();
return PyValue::False();
```

---

## Available Bindings

### Core Functions (Room & Camera)

| Function | Parameters | Description |
|----------|------------|-------------|
| `change_room(room, fade)` | room: EscValue, fade: int | Switch to different room |
| `camera_follow(actor)` | actor: EscValue | Make camera follow actor |
| `camera_at(position)` | position: int or EscValue | Set camera position |
| `camera_pan_to(target)` | target: EscValue | Pan camera smoothly |
| `put_at(obj, x, y, room)` | obj: EscValue, x/y: int, room: EscValue | Position object |

### UI Functions (Dialog & Text)

| Function | Parameters | Description |
|----------|------------|-------------|
| `say_line(text)` or `say_line(actor, text, wait, duration)` | text: String | Display dialog |
| `print_line(text, x, y, color, shadow, centered, width, outline)` | Various | Print text at position |

### Game Logic Functions

| Function | Parameters | Description |
|----------|------------|-------------|
| `break_time(frames)` | frames: int (default 1) | Wait N frames |
| `cutscene(type, setup_fn, cleanup_fn)` | type: int, fn: Python function | Start cutscene |
| `pickup_obj(obj, actor)` | obj: EscValue, actor: EscValue | Add to inventory |

### Drawing Functions

| Function | Parameters | Description |
|----------|------------|-------------|
| `set_trans_col(color, enable)` | color: int, enable: bool | Set transparent color |
| `map_draw(src_x, src_y, dest_x, dest_y, width, height, flags)` | Various | Draw map tiles |

---

## Lambda/Callback Support

Python functions can be passed as callbacks to C++ functions:

```python
# Python code
def setup_cutscene():
    say_line("Cutscene dialog")
    print("Setting up cutscene...")

cutscene(1, setup_cutscene, None)

# Or with inline lambda
cutscene(1, lambda: (
    say_line("Hello"),
    print("World")
))
```

In C++, check if argument is a function and call it:

```cpp
if(args[1].IsFunction()) {
    PyValue fn = args[1];
    // Store for later execution or call immediately:
    // vm.Call(fn, Vector<PyValue>());
}
```

---

## Object References

Game objects (rooms, actors, items) are represented as `EscValue` in C++.

### ESC Object Syntax
```esc
:my_room = {
    "map": [0, 0, 10, 10],
    "objects": [:actor1, :actor2]
}

:actor1 = {
    "name": "Hero",
    "x": 100,
    "y": 200
}
```

### Python Usage
```python
# Objects are passed by reference
camera_follow(my_actor)
put_at(my_object, 50, 60, my_room)

# Object properties can be accessed (needs wrapper implementation)
# x = my_actor.x
# my_actor.x = 100
```

---

## Error Handling

Use Python exceptions via the VM:

```cpp
if(args.GetCount() < 1) {
    vm.RaiseException("function_name requires at least 1 argument");
    return PyValue::None();
}

if(!args[0].IsStr()) {
    vm.RaiseException("argument 1 must be a string");
    return PyValue::None();
}
```

---

## Testing

### Build Test
```bash
bin/build -j12 Adventure
```

### Python Test
```bash
# Run the test script
python3 uppsrc/Adventure/test_bindings.py
```

### Integration Test
```python
# In a Python script loaded by PyVM
from Adventure import *

# Test basic functions
say_line("Hello from Python!")
camera_follow(player_actor)
break_time(30)
```

---

## Implementation Status

### Completed
- [x] Binding infrastructure (AdventureBindings.h/cpp)
- [x] Core function bindings (5 functions)
- [x] UI function bindings (2 functions)
- [x] Game logic bindings (3 functions)
- [x] Drawing function bindings (2 functions)
- [x] Test script (test_bindings.py)

### TODO
- [ ] Integrate bindings with Adventure engine initialization
- [ ] Implement EscValue wrapper class for Python
- [ ] Add property access for game objects (obj.x, obj.y, etc.)
- [ ] Implement lambda/callback execution in cutscene
- [ ] Add remaining functions from function-catalog.md
- [ ] Create comprehensive integration tests
- [ ] Document object reference handling

---

## Development Guidelines

### Adding New Bindings

1. **Declare in header** (`AdventureBindings.h`):
   ```cpp
   static PyValue new_function(const Vector<PyValue>& args, void* user_data);
   ```

2. **Implement in cpp** (`AdventureBindings.cpp`):
   ```cpp
   PyValue AdventureBindings::new_function(const Vector<PyValue>& args, void* user_data)
   {
       Program* prog = GetProgram(user_data);
       // Validate, extract, call, return
   }
   ```

3. **Register** in `RegisterAll()`:
   ```cpp
   globals.SetItem(PyValue("new_function"), 
                   PyValue::Function("new_function", new_function, &prog));
   ```

4. **Test** in `test_bindings.py`:
   ```python
   new_function(arg1, arg2)
   ```

### Code Style

- Use `GetProgram(user_data)` helper to extract Program instance
- Validate argument count before accessing args
- Use `PyToEscValue()` helper for object conversion
- Return `PyValue::None()` for void functions
- Add comments explaining ESC equivalent

---

## References

- **Function Catalog**: `plan/original-fix/01-bindings/function-catalog.md`
- **PyVM API**: `../ByteVM/PyVM.h`
- **PyValue API**: `../ByteVM/PyValue.h`
- **Binding Helpers**: `../ByteVM/PyBindings.h`
- **Example Pattern**: `../ScriptCommon/CardGamePlugin.cpp:683` (SyncBindings)

---

## Notes

1. **EscValue Handling**: Currently uses a simple conversion. For full object property access, need a proper EscValue wrapper class.

2. **Lambda Storage**: Functions passed as lambdas need to be stored for later execution (e.g., in cutscene cleanup).

3. **Script Integration**: Bindings should integrate with the existing ESC script system, not replace it.

4. **Thread Safety**: PyVM calls should happen on the main thread. Use proper synchronization if called from other threads.

5. **Memory Management**: PyValue uses reference counting. Be careful with object lifetimes, especially for callbacks.
