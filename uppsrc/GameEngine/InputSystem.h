#ifndef UPP_INPUTSYSTEM_H
#define UPP_INPUTSYSTEM_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <api/Hal/Hal.h>  // HAL API includes

NAMESPACE_UPP

// Enum for gamepad buttons (mapping to common gamepad layout)
enum GamepadButton {
	BTN_A = 0,       // Usually bottom button
	BTN_B = 1,       // Usually right button
	BTN_X = 2,       // Usually left button
	BTN_Y = 3,       // Usually top button
	BTN_LB = 4,      // Left bumper
	BTN_RB = 5,      // Right bumper
	BTN_BACK = 6,    // Back button
	BTN_START = 7,   // Start button
	BTN_L3 = 8,      // Left stick press
	BTN_R3 = 9,      // Right stick press
	BTN_GUIDE = 10,  // Guide button (Xbox logo)
	BTN_DPAD_UP = 11,
	BTN_DPAD_DOWN = 12,
	BTN_DPAD_LEFT = 13,
	BTN_DPAD_RIGHT = 14,
};

// Enum for gamepad axes
enum GamepadAxis {
	AXIS_LEFT_X = 0,   // Left stick X
	AXIS_LEFT_Y = 1,   // Left stick Y
	AXIS_RIGHT_X = 2,  // Right stick X
	AXIS_RIGHT_Y = 3,  // Right stick Y
	AXIS_LEFT_TRIG = 4, // Left trigger
	AXIS_RIGHT_TRIG = 5, // Right trigger
};

// Structure to hold gamepad state
struct GamepadState {
	bool buttons[15] = {false};  // All buttons, indexed by GamepadButton
	double axes[6] = {0.0};      // All axes, indexed by GamepadAxis
	bool connected = false;      // Whether this gamepad is connected

	void Clear() {
		for (int i = 0; i < 15; i++) buttons[i] = false;
		for (int i = 0; i < 6; i++) axes[i] = 0.0;
		connected = false;
	}
};

// Input state structure
struct InputState {
	// Keyboard state
	bool keys[256] = {false};   // Key states (simplified key mapping)

	// Mouse state
	Point mouse_pos;            // Current mouse position
	bool mouse_buttons[3] = {false};  // Left, right, middle
	int mouse_wheel = 0;        // Mouse wheel delta

	// Gamepad states (support up to 4 gamepads)
	GamepadState gamepads[4];

	void Clear() {
		for (int i = 0; i < 256; i++) keys[i] = false;
		mouse_pos = Point(0, 0);
		for (int i = 0; i < 3; i++) mouse_buttons[i] = false;
		mouse_wheel = 0;

		for (int i = 0; i < 4; i++) {
			gamepads[i].Clear();
		}
	}
};

// Input system class that integrates with HAL
class InputSystem {
public:
	InputSystem();
	virtual ~InputSystem();

	// Initialize the input system with HAL events
	bool Initialize(AtomBase& hal_context, AtomBase& hal_events);
	void Uninitialize();

	// Update input state from HAL events
	void Update(double dt);

	// Process a single event from HAL
	void ProcessEvent(const GeomEvent& event);

	// Get current input state
	const InputState& GetState() const { return current_state; }

	// Check keyboard state
	bool IsKeyDown(dword key) const;
	bool IsKeyPressed(dword key) const; // Key pressed this frame
	bool IsKeyReleased(dword key) const; // Key released this frame

	// Check mouse state
	bool IsMouseButtonDown(int button) const; // 0=left, 1=right, 2=middle
	bool IsMouseButtonPressed(int button) const; // Button pressed this frame
	bool IsMouseButtonReleased(int button) const; // Button released this frame
	Point GetMousePosition() const { return current_state.mouse_pos; }
	int GetMouseWheel() const { return current_state.mouse_wheel; }

	// Check gamepad state
	bool IsGamepadButtonDown(int gamepad_index, GamepadButton button) const;
	bool IsGamepadButtonPressed(int gamepad_index, GamepadButton button) const;
	bool IsGamepadAxisMoved(int gamepad_index, GamepadAxis axis, double threshold = 0.1) const;
	double GetGamepadAxisValue(int gamepad_index, GamepadAxis axis) const;
	bool IsGamepadConnected(int gamepad_index) const;

	// Get previous frame input state (for checking pressed/released states)
	const InputState& GetPreviousState() const { return previous_state; }

	// Direct access to HAL components
	void SetEventsBase(AtomBase* hal_events);
	AtomBase* GetEventsBase() const { return events_base; }

private:
	// Current and previous input states for detecting pressed/released states
	InputState current_state;
	InputState previous_state;

	// HAL components
	AtomBase* context_base = nullptr;
	AtomBase* events_base = nullptr;

	// Internal state tracking
	Vector<GeomEvent> pending_events;

	// Process different types of events
	void ProcessKeyDownEvent(const GeomEvent& event);
	void ProcessKeyUpEvent(const GeomEvent& event);
	void ProcessMouseMoveEvent(const GeomEvent& event);
	void ProcessMouseDownEvent(const GeomEvent& event);
	void ProcessMouseUpEvent(const GeomEvent& event);
	void ProcessMouseWheelEvent(const GeomEvent& event);
	void ProcessWindowEvent(const GeomEvent& event);
	
	// Gamepad handling
	void UpdateGamepads();
};

END_UPP_NAMESPACE

#endif