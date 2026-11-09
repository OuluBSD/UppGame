#ifndef _GameCommon_ActionExecutor_h_
#define _GameCommon_ActionExecutor_h_

#include <Core/Core.h>

namespace Upp {

class ActionExecutor {
	bool enabled = false;
	Rect window_rect;

public:
	ActionExecutor();
	~ActionExecutor();

	void SetEnabled(bool e) { enabled = e; }
	void SetWindowRect(const Rect& r) { window_rect = r; }

	bool Click(int x, int y);
	bool ClickRelative(int rx, int ry, int rw, int rh); // Relative to window_rect
	
	bool ExecuteAction(const String& action, const String& platform, const Rect& win_rect);
};

}

#endif
