#ifndef _Platformer2D_Platform_Input_h_
#define _Platformer2D_Platform_Input_h_

#include "Platform.h"
#include <game/Platformer2D/Core/Core.h>
#include <CtrlLib/CtrlLib.h>

NAMESPACE_UPP

// ============================================================================
// Key mapping structure
// ============================================================================

struct KeyMapping
{
	ScanCode jump;
	ScanCode pogo;
	ScanCode fire;
	ScanCode upLeft;
	ScanCode up;
	ScanCode upRight;
	ScanCode left;
	ScanCode right;
	ScanCode downLeft;
	ScanCode down;
	ScanCode downRight;
};

// ============================================================================
// Input Mapper (CtrlLib → ControlFrame)
// ============================================================================

class InputMapper
{
public:
	static void Initialize();
	static void Shutdown();
	
	// Called from window events
	static void OnKeyDown(dword key);
	static void OnKeyUp(dword key);
	static void OnMouseMove(Point p);
	static void OnMouseEvent(int event, Point p, int zdelta);
	
	// Read current control state (for simulation)
	static void ReadControls(int player, ControlFrame* controls);
	
	// Configuration
	static void LoadKeyConfig();
	static void SaveKeyConfig();
	static void SetDefaultKeys();
	
private:
	static KeyMapping s_keyMap;
	static ControlFrame s_currentFrame;
	static bool s_initialized;
	
	// Key mapping: U++ dword -> ScanCode
	static ScanCode MapKey(dword key);
};

END_UPP_NAMESPACE

#endif // _Platformer2D_Platform_Input_h_