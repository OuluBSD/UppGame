#ifndef _SweetSandy_Settings_h_
#define _SweetSandy_Settings_h_

// Rebindable driving keys. One key per action (not "arrows OR wasd" like
// the original hardcoded PlayScreen::Key did) - simpler to show/edit in a
// settings screen, and the default still matches the old arrow-key layout.
struct KeyBindings {
	dword throttle    = K_UP;
	dword brake       = K_DOWN;
	dword steer_left  = K_LEFT;
	dword steer_right = K_RIGHT;
	dword back        = K_ESCAPE; // return to the menu from PlayScreen
};

// Process-wide bindings, lazily loaded from AppRegistry on first access
// (see ai/skill/app-registry.md) and cached in memory after that -
// SaveKeyBindings() must be called after changing a field to persist it.
KeyBindings& GetKeyBindings();
void SaveKeyBindings();

// Short display name for a key code - "Up", "W", "Esc", or "Key <code>" as
// a fallback. Used by SettingsScreen to label each rebind button.
String KeyName(dword key);

#endif
