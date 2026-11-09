#ifndef _SweetSandy_AppShell_h_
#define _SweetSandy_AppShell_h_

// Screen ids used with AppShell::ShowScreen.
#define SCREEN_MENU        "menu"
#define SCREEN_NEWGAME     "newgame"
#define SCREEN_SETTINGS    "settings"
#define SCREEN_MULTIPLAYER "multiplayer"
#define SCREEN_CREDITS     "credits"
#define SCREEN_TUTORIAL    "tutorial"
#define SCREEN_PLAY        "play"

class MainMenuScreen;
class NewGameScreen;
class PlaceholderScreen;
class SettingsScreen;
class PlayScreen;
class MapEditorWindow;

// Owns one header-less TabCtrl that all top-level screens are added to.
// Screen switching goes through ShowScreen() instead of Ctrl::Show/Hide,
// per the gui-user-gate TabCtrl convention (see AGENTS.md).
class AppShell : public TopWindow {
	TabCtrl shell;
	VectorMap<String, Ctrl*> screen;

	One<MainMenuScreen>    menu_screen;
	One<NewGameScreen>     newgame_screen;
	One<SettingsScreen>    settings_screen;
	One<PlaceholderScreen> multiplayer_screen;
	One<PlaceholderScreen> credits_screen;
	One<PlaceholderScreen> tutorial_screen;
	One<PlayScreen>        play_screen;

	// Map Editor phase, Task 1: lazily created on the first F1 press,
	// see ToggleMapEditor. A genuinely separate TopWindow, not one of
	// the `shell` TabCtrl's screens.
	One<MapEditorWindow>   map_editor;

	void AddScreen(const char *id, Ctrl& ctrl, const char *tab_text);

public:
	typedef AppShell CLASSNAME;

	String selected_track_id;
	String selected_vehicle_id;

	AppShell();

	void ShowScreen(const String& id);

	// F1 opens the map editor as a view-only window if not already
	// open; a second press closes it. Public so --mapeditortest can
	// drive it directly instead of synthesizing a real key event, same
	// style as --menutest's own direct ShowScreen() calls.
	void ToggleMapEditor();

	bool Key(dword key, int count) override;

	// Drives the same path as NewGameScreen's "Aloita" button. Asserts if
	// either id is not in the catalog. Used by --play [track] [vehicle] and
	// by the --playtest headless smoke test.
	void StartRunById(const String& track_id, const String& vehicle_id);

	// StartRunById with the first catalog entries.
	void StartDefaultRun();

	// For --citypic's one-shot framebuffer dump request. Defined in the
	// .cpp, not inline here - PlayScreen is only forward-declared in this
	// header, so dereferencing One<PlayScreen> needs its complete
	// definition, which this header doesn't (and shouldn't) include.
	PlayScreen& GetPlayScreen();

	// Map Editor phase, Task 10: test-only access for --mapeditortest,
	// same "defined in .cpp, not inline" reasoning as GetPlayScreen
	// above. Asserts if ToggleMapEditor hasn't been called yet (map_editor
	// is still null) - callers open it first, same as GetPlayScreen's own
	// implicit "a run must have started" precondition.
	MapEditorWindow& GetMapEditor();
};

#endif
