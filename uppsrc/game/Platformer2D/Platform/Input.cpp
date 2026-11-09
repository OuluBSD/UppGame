#include "Input.h"

NAMESPACE_UPP

KeyMapping InputMapper::s_keyMap;
ControlFrame InputMapper::s_currentFrame;
bool InputMapper::s_initialized = false;

void InputMapper::Initialize()
{
	if (s_initialized) return;
	
	SetDefaultKeys();
	LoadKeyConfig();
	
	s_currentFrame = ControlFrame();
	s_currentFrame.xDirection = Motion::Motion_None;
	s_currentFrame.yDirection = Motion::Motion_None;
	s_currentFrame.dir = InputDirection::Dir_None;
	
	s_initialized = true;
	LOG("InputMapper::Initialize");
}

void InputMapper::Shutdown()
{
	if (!s_initialized) return;
	SaveKeyConfig();
	s_initialized = false;
}

void InputMapper::OnKeyDown(dword key)
{
	ScanCode sc = MapKey(key);
	if (sc == SC_None) return;
	
	// Map to control frame
	if (sc == s_keyMap.jump) s_currentFrame.jump = true;
	else if (sc == s_keyMap.pogo) s_currentFrame.pogo = true;
	else if (sc == s_keyMap.fire) s_currentFrame.button2 = true;
	else if (sc == s_keyMap.upLeft) { s_currentFrame.xDirection = Motion::Motion_Left; s_currentFrame.yDirection = Motion::Motion_Up; s_currentFrame.dir = InputDirection::Dir_NorthWest; }
	else if (sc == s_keyMap.up) { s_currentFrame.yDirection = Motion::Motion_Up; s_currentFrame.dir = InputDirection::Dir_North; }
	else if (sc == s_keyMap.upRight) { s_currentFrame.xDirection = Motion::Motion_Right; s_currentFrame.yDirection = Motion::Motion_Up; s_currentFrame.dir = InputDirection::Dir_NorthEast; }
	else if (sc == s_keyMap.left) { s_currentFrame.xDirection = Motion::Motion_Left; s_currentFrame.dir = InputDirection::Dir_West; }
	else if (sc == s_keyMap.right) { s_currentFrame.xDirection = Motion::Motion_Right; s_currentFrame.dir = InputDirection::Dir_East; }
	else if (sc == s_keyMap.downLeft) { s_currentFrame.xDirection = Motion::Motion_Left; s_currentFrame.yDirection = Motion::Motion_Down; s_currentFrame.dir = InputDirection::Dir_SouthWest; }
	else if (sc == s_keyMap.down) { s_currentFrame.yDirection = Motion::Motion_Down; s_currentFrame.dir = InputDirection::Dir_South; }
	else if (sc == s_keyMap.downRight) { s_currentFrame.xDirection = Motion::Motion_Right; s_currentFrame.yDirection = Motion::Motion_Down; s_currentFrame.dir = InputDirection::Dir_SouthEast; }
}

void InputMapper::OnKeyUp(dword key)
{
	ScanCode sc = MapKey(key);
	if (sc == SC_None) return;
	
	if (sc == s_keyMap.jump) s_currentFrame.jump = false;
	else if (sc == s_keyMap.pogo) s_currentFrame.pogo = false;
	else if (sc == s_keyMap.fire) s_currentFrame.button2 = false;
	else if (sc == s_keyMap.upLeft || sc == s_keyMap.left || sc == s_keyMap.downLeft)
	{
		s_currentFrame.xDirection = Motion::Motion_None;
		if (s_currentFrame.yDirection == Motion::Motion_None) s_currentFrame.dir = InputDirection::Dir_None;
	}
	else if (sc == s_keyMap.upRight || sc == s_keyMap.right || sc == s_keyMap.downRight)
	{
		s_currentFrame.xDirection = Motion::Motion_None;
		if (s_currentFrame.yDirection == Motion::Motion_None) s_currentFrame.dir = InputDirection::Dir_None;
	}
	else if (sc == s_keyMap.up)
	{
		s_currentFrame.yDirection = Motion::Motion_None;
		if (s_currentFrame.xDirection == Motion::Motion_None) s_currentFrame.dir = InputDirection::Dir_None;
	}
	else if (sc == s_keyMap.down)
	{
		s_currentFrame.yDirection = Motion::Motion_None;
		if (s_currentFrame.xDirection == Motion::Motion_None) s_currentFrame.dir = InputDirection::Dir_None;
	}
}

void InputMapper::OnMouseMove(Point p)
{
	s_currentFrame.x = p.x;
	s_currentFrame.y = p.y;
}

void InputMapper::OnMouseEvent(int event, Point p, int zdelta)
{
	s_currentFrame.x = p.x;
	s_currentFrame.y = p.y;
	
	if (event == 1) // Left down
		s_currentFrame.jump = true;
	else if (event == 2) // Left up
		s_currentFrame.jump = false;
	else if (event == 4) // Right down
		s_currentFrame.fire = true;
	else if (event == 8) // Right up
		s_currentFrame.fire = false;
}

void InputMapper::ReadControls(int player, ControlFrame* controls)
{
	if (!controls) return;
	if (player == 0)
		*controls = s_currentFrame;
	else
		controls->Clear(); // Only player 0 for now
}

ScanCode InputMapper::MapKey(dword key)
{
	// Map U++ Key to ScanCode (matching OmniSpeak scancodes)
	// This is a simplified mapping
	switch (key)
	{
	case K_ESCAPE: return SC_Escape;
	case K_F1: return SC_F1;
	case K_F2: return SC_F2;
	case K_F3: return SC_F3;
	case K_F4: return SC_F4;
	case K_F5: return SC_F5;
	case K_F6: return SC_F6;
	case K_F7: return SC_F7;
	case K_F8: return SC_F8;
	case K_F9: return SC_F9;
	case K_F10: return SC_F10;
	case K_F11: return SC_F11;
	case K_F12: return SC_F12;
	case K_UP: return SC_UpArrow;
	case K_DOWN: return SC_DownArrow;
	case K_LEFT: return SC_LeftArrow;
	case K_RIGHT: return SC_RightArrow;
	case K_SPACE: return SC_Space;
	case K_CTRL: return SC_Control;
	case K_ALT: return SC_Alt;
	case K_SHIFT: return SC_LeftShift;
	case K_ENTER: return SC_Enter;
	case K_TAB: return SC_Tab;
	case K_BACKSPACE: return SC_Backspace;
	case 'A': return SC_A;
	case 'B': return SC_B;
	case 'C': return SC_C;
	case 'D': return SC_D;
	case 'E': return SC_E;
	case 'F': return SC_F;
	case 'G': return SC_G;
	case 'H': return SC_H;
	case 'I': return SC_I;
	case 'J': return SC_J;
	case 'K': return SC_K;
	case 'L': return SC_L;
	case 'M': return SC_M;
	case 'N': return SC_N;
	case 'O': return SC_O;
	case 'P': return SC_P;
	case 'Q': return SC_Q;
	case 'R': return SC_R;
	case 'S': return SC_S;
	case 'T': return SC_T;
	case 'U': return SC_U;
	case 'V': return SC_V;
	case 'W': return SC_W;
	case 'X': return SC_X;
	case 'Y': return SC_Y;
	case 'Z': return SC_Z;
	case '0': return SC_Zero;
	case '1': return SC_One;
	case '2': return SC_Two;
	case '3': return SC_Three;
	case '4': return SC_Four;
	case '5': return SC_Five;
	case '6': return SC_Six;
	case '7': return SC_Seven;
	case '8': return SC_Eight;
	case '9': return SC_Nine;
	default: return SC_None;
	}
}

void InputMapper::SetDefaultKeys()
{
	s_keyMap.jump = SC_Space;
	s_keyMap.pogo = SC_Control;
	s_keyMap.fire = SC_Alt;
	s_keyMap.upLeft = SC_Home;
	s_keyMap.up = SC_UpArrow;
	s_keyMap.upRight = SC_PgUp;
	s_keyMap.left = SC_LeftArrow;
	s_keyMap.right = SC_RightArrow;
	s_keyMap.downLeft = SC_End;
	s_keyMap.down = SC_DownArrow;
	s_keyMap.downRight = SC_PgDown;
}

void InputMapper::LoadKeyConfig()
{
	// TODO: Load from config file
}

void InputMapper::SaveKeyConfig()
{
	// TODO: Save to config file
}

END_UPP_NAMESPACE