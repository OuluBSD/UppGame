#include <Poker/ActionExecutor.h>

#ifdef PLATFORM_LINUX
// Prevent clashes with Upp::Font/Upp::Time/Upp::Display in mixed-blitz builds.
#define Font X11_Font
#define Time X11_Time
#define Display X11_Display
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#undef Display
#undef Time
#undef Font
#endif

namespace Upp {

ActionExecutor::ActionExecutor() {
}

ActionExecutor::~ActionExecutor() {
}

bool ActionExecutor::Click(int x, int y) {
	if (!enabled) return false;

#ifdef PLATFORM_LINUX
	::X11_Display* dpy = XOpenDisplay(NULL);
	if (!dpy) return false;

	XTestFakeMotionEvent(dpy, -1, x, y, CurrentTime);
	XTestFakeButtonEvent(dpy, 1, True, CurrentTime);
	XTestFakeButtonEvent(dpy, 1, False, CurrentTime);

	XFlush(dpy);
	XCloseDisplay(dpy);
	return true;
#else
	return false;
#endif
}

bool ActionExecutor::ClickRelative(int rx, int ry, int rw, int rh) {
	if (window_rect.IsEmpty()) return false;
	int x = window_rect.left + rx * window_rect.GetWidth() / rw;
	int y = window_rect.top + ry * window_rect.GetHeight() / rh;
	return Click(x, y);
}

bool ActionExecutor::ExecuteAction(const String& action, const String& platform, const Rect& win_rect) {
	if (!enabled) return false;
	SetWindowRect(win_rect);
	
	const int base_w = 1920;
	const int base_h = 1080;

	if (action == "fold") return ClickRelative(750, 1030, base_w, base_h);
	if (action == "check" || action == "call") return ClickRelative(960, 1030, base_w, base_h);
	if (action == "bet" || action == "raise" || action == "allin") return ClickRelative(1170, 1030, base_w, base_h);

	return false;
}

}
