#include "InputSystem.h"
#include <Eon/Eon.h>  // For event handling

NAMESPACE_UPP

InputSystem::InputSystem() {
	current_state.Clear();
	previous_state.Clear();
}

InputSystem::~InputSystem() {
	Uninitialize();
}

bool InputSystem::Initialize(AtomBase& hal_context, AtomBase& hal_events) {
	context_base = &hal_context;
	events_base = &hal_events;

	// Initialize gamepads state
	for (int i = 0; i < 4; i++) {
		current_state.gamepads[i].Clear();
		previous_state.gamepads[i].Clear();
	}

	return true;
}

void InputSystem::Uninitialize() {
	context_base = nullptr;
	events_base = nullptr;
}

void InputSystem::Update(double dt) {
	// Store previous state for pressed/released detection
	previous_state = current_state;

	// Process pending events
	if (events_base && events_base->IsInitialized()) {
		// Check if there are events ready from the HAL EventsBase
		PacketIO io;
		if (events_base->IsReady(io)) {
			// Create a packet to receive event data
			RealtimeSourceConfig cfg;
			PacketValue packet;
			ValueFormat fmt;
			fmt.SetEvent(DevCls::EVENT);
			packet.SetFormat(fmt);

			if (events_base->Send(cfg, packet, 0)) {
				// Process the received events
				GeomEventCollection& events = packet.SetData<GeomEventCollection>();

				for (const GeomEvent& event : events) {
					ProcessEvent(event);
				}
			}
		}

		// Query HAL for actual gamepad states
		// For now, implementing a basic approach - in a full implementation
		// we would query the HAL for gamepad states directly
		UpdateGamepads();
	}
}

void InputSystem::UpdateGamepads() {
	// In a real HAL-based implementation, we would query the HAL for gamepad state
	// For now, we'll simulate gamepad connection based on any gamepad-related events
	// This is a placeholder - in a real implementation, we would interface with HAL
	
	// For demonstration purposes, assume gamepads 0 and 1 are connected
	// In a real implementation, we would query HAL for actual connection status
	current_state.gamepads[0].connected = true;  // Simulating connected gamepad
	current_state.gamepads[1].connected = true;  // Simulating connected gamepad
}

void InputSystem::ProcessEvent(const GeomEvent& event) {
	switch (event.type) {
		case EVENT_KEYDOWN:
			ProcessKeyDownEvent(event);
			break;
		case EVENT_KEYUP:
			ProcessKeyUpEvent(event);
			break;
		case EVENT_MOUSEMOVE:
			ProcessMouseMoveEvent(event);
			break;
		case EVENT_MOUSE_EVENT:
			if (event.n == MOUSE_LEFTDOWN || event.n == MOUSE_MIDDLEDOWN || event.n == MOUSE_RIGHTDOWN) {
				ProcessMouseDownEvent(event);
			} else if (event.n == MOUSE_LEFTUP || event.n == MOUSE_MIDDLEUP || event.n == MOUSE_RIGHTUP) {
				ProcessMouseUpEvent(event);
			}
			break;
		case EVENT_MOUSEWHEEL:
			ProcessMouseWheelEvent(event);
			break;
		case EVENT_WINDOW_RESIZE:
			ProcessWindowEvent(event);
			break;
		default:
			// Other event types are ignored by the input system
			// This is where we could add gamepad event handling in the future
			// Such as EVENT_GAMEPAD_CONNECTED, EVENT_GAMEPAD_DISCONNECTED,
			// EVENT_GAMEPAD_BUTTON_DOWN, etc.
			break;
	}
}

void InputSystem::ProcessKeyDownEvent(const GeomEvent& event) {
	// Extract key code and handle modifier keys
	dword key = event.value & ~I_KEYUP;  // Remove I_KEYUP flag if present

	// Map common keys to simple indices
	int idx = 0;
	if (key >= 'A' && key <= 'Z') {
		idx = key - 'A' + 65;  // Map A-Z to indices 65-90
	} else if (key >= '0' && key <= '9') {
		idx = key - '0' + 48;  // Map 0-9 to indices 48-57
	} else if (key >= I_F1 && key <= I_F12) {
		idx = key - I_F1 + 112; // Map F1-F12 to indices 112-123
	} else {
		// Map other special keys
		switch (key) {
			case I_SPACE: idx = 32; break;
			case I_RETURN: case I_ENTER: idx = 13; break;
			case I_ESCAPE: idx = 27; break;
			case I_BACKSPACE: case I_BACK: idx = 8; break;
			case I_TAB: idx = 9; break;
			case I_LEFT: case I_RIGHT: case I_UP: case I_DOWN:
				idx = key; break; // Use the original key code directly
			default: idx = key; break;
		}
	}

	if (idx >= 0 && idx < 256) {
		current_state.keys[idx] = true;
	}
}

void InputSystem::ProcessKeyUpEvent(const GeomEvent& event) {
	// Extract key code and handle modifier keys
	dword key = event.value & ~I_KEYUP;  // Remove I_KEYUP flag if present

	// Map common keys to simple indices (same as key down)
	int idx = 0;
	if (key >= 'A' && key <= 'Z') {
		idx = key - 'A' + 65;  // Map A-Z to indices 65-90
	} else if (key >= '0' && key <= '9') {
		idx = key - '0' + 48;  // Map 0-9 to indices 48-57
	} else if (key >= I_F1 && key <= I_F12) {
		idx = key - I_F1 + 112; // Map F1-F12 to indices 112-123
	} else {
		// Map other special keys
		switch (key) {
			case I_SPACE: idx = 32; break;
			case I_RETURN: case I_ENTER: idx = 13; break;
			case I_ESCAPE: idx = 27; break;
			case I_BACKSPACE: case I_BACK: idx = 8; break;
			case I_TAB: idx = 9; break;
			case I_LEFT: case I_RIGHT: case I_UP: case I_DOWN:
				idx = key; break; // Use the original key code directly
			default: idx = key; break;
		}
	}

	if (idx >= 0 && idx < 256) {
		current_state.keys[idx] = false;
	}
}

void InputSystem::ProcessMouseMoveEvent(const GeomEvent& event) {
	current_state.mouse_pos = event.pt;
}

void InputSystem::ProcessMouseDownEvent(const GeomEvent& event) {
	current_state.mouse_pos = event.pt;
	switch (event.n) {
		case MOUSE_LEFTDOWN:
			current_state.mouse_buttons[0] = true;
			break;
		case MOUSE_RIGHTDOWN:
			current_state.mouse_buttons[1] = true;
			break;
		case MOUSE_MIDDLEDOWN:
			current_state.mouse_buttons[2] = true;
			break;
	}
}

void InputSystem::ProcessMouseUpEvent(const GeomEvent& event) {
	current_state.mouse_pos = event.pt;
	switch (event.n) {
		case MOUSE_LEFTUP:
			current_state.mouse_buttons[0] = false;
			break;
		case MOUSE_RIGHTUP:
			current_state.mouse_buttons[1] = false;
			break;
		case MOUSE_MIDDLEUP:
			current_state.mouse_buttons[2] = false;
			break;
	}
}

void InputSystem::ProcessMouseWheelEvent(const GeomEvent& event) {
	current_state.mouse_pos = event.pt;
	current_state.mouse_wheel = event.n;  // Delta value
}

void InputSystem::ProcessWindowEvent(const GeomEvent& event) {
	// Window resize event - could be used to update rendering viewport
	// For input handling, we don't need special handling
}

bool InputSystem::IsKeyDown(dword key) const {
	int idx = -1;
	if (key >= 'A' && key <= 'Z') {
		idx = key - 'A' + 65;
	} else if (key >= '0' && key <= '9') {
		idx = key - '0' + 48;
	} else if (key >= I_F1 && key <= I_F12) {
		idx = key - I_F1 + 112;
	} else {
		switch (key) {
			case I_SPACE: idx = 32; break;
			case I_RETURN: case I_ENTER: idx = 13; break;
			case I_ESCAPE: idx = 27; break;
			case I_BACKSPACE: case I_BACK: idx = 8; break;
			case I_TAB: idx = 9; break;
			case I_LEFT: case I_RIGHT: case I_UP: case I_DOWN:
				idx = key; break;
			default: idx = key; break;
		}
	}

	if (idx >= 0 && idx < 256) {
		return current_state.keys[idx];
	}
	return false;
}

bool InputSystem::IsKeyPressed(dword key) const {
	// Check if key is down now but was not down in the previous frame
	return IsKeyDown(key) && !previous_state.keys[key >= 'A' && key <= 'Z' ? key - 'A' + 65 :
	                        (key >= '0' && key <= '9' ? key - '0' + 48 :
	                        (key >= I_F1 && key <= I_F12 ? key - I_F1 + 112 :
	                        (key == I_SPACE ? 32 :
	                        (key == I_RETURN || key == I_ENTER ? 13 :
	                        (key == I_ESCAPE ? 27 :
	                        (key == I_BACKSPACE || key == I_BACK ? 8 :
	                        (key == I_TAB ? 9 :
	                        (key == I_LEFT || key == I_RIGHT || key == I_UP || key == I_DOWN ? key : key))))))))];
}

bool InputSystem::IsKeyReleased(dword key) const {
	// Check if key was down in the previous frame but is not down now
	int idx = -1;
	if (key >= 'A' && key <= 'Z') {
		idx = key - 'A' + 65;
	} else if (key >= '0' && key <= '9') {
		idx = key - '0' + 48;
	} else if (key >= I_F1 && key <= I_F12) {
		idx = key - I_F1 + 112;
	} else {
		switch (key) {
			case I_SPACE: idx = 32; break;
			case I_RETURN: case I_ENTER: idx = 13; break;
			case I_ESCAPE: idx = 27; break;
			case I_BACKSPACE: case I_BACK: idx = 8; break;
			case I_TAB: idx = 9; break;
			case I_LEFT: case I_RIGHT: case I_UP: case I_DOWN:
				idx = key; break;
			default: idx = key; break;
		}
	}

	if (idx >= 0 && idx < 256) {
		return !current_state.keys[idx] && previous_state.keys[idx];
	}
	return false;
}

bool InputSystem::IsMouseButtonDown(int button) const {
	if (button >= 0 && button < 3) {
		return current_state.mouse_buttons[button];
	}
	return false;
}

bool InputSystem::IsMouseButtonPressed(int button) const {
	if (button >= 0 && button < 3) {
		return current_state.mouse_buttons[button] && !previous_state.mouse_buttons[button];
	}
	return false;
}

bool InputSystem::IsMouseButtonReleased(int button) const {
	if (button >= 0 && button < 3) {
		return !current_state.mouse_buttons[button] && previous_state.mouse_buttons[button];
	}
	return false;
}

bool InputSystem::IsGamepadButtonDown(int gamepad_index, GamepadButton button) const {
	if (gamepad_index >= 0 && gamepad_index < 4) {
		return current_state.gamepads[gamepad_index].buttons[button];
	}
	return false;
}

bool InputSystem::IsGamepadButtonPressed(int gamepad_index, GamepadButton button) const {
	if (gamepad_index >= 0 && gamepad_index < 4) {
		return current_state.gamepads[gamepad_index].buttons[button] &&
		       !previous_state.gamepads[gamepad_index].buttons[button];
	}
	return false;
}

bool InputSystem::IsGamepadAxisMoved(int gamepad_index, GamepadAxis axis, double threshold) const {
	if (gamepad_index >= 0 && gamepad_index < 4) {
		double value = current_state.gamepads[gamepad_index].axes[axis];
		return (value > threshold || value < -threshold);
	}
	return false;
}

double InputSystem::GetGamepadAxisValue(int gamepad_index, GamepadAxis axis) const {
	if (gamepad_index >= 0 && gamepad_index < 4) {
		return current_state.gamepads[gamepad_index].axes[axis];
	}
	return 0.0;
}

bool InputSystem::IsGamepadConnected(int gamepad_index) const {
	if (gamepad_index >= 0 && gamepad_index < 4) {
		return current_state.gamepads[gamepad_index].connected;
	}
	return false;
}

void InputSystem::SetEventsBase(AtomBase* hal_events) {
	events_base = hal_events;
}

END_UPP_NAMESPACE