#ifndef _SweetSandy_MainMenuScreen_h_
#define _SweetSandy_MainMenuScreen_h_

// Top-level main menu: Tutorial, New game, Settings, Multiplayer, Credits, Exit.
class MainMenuScreen : public ParentCtrl {
	Label  title;
	Button tutorial_button;
	Button newgame_button;
	Button settings_button;
	Button multiplayer_button;
	Button credits_button;
	Button exit_button;

public:
	typedef MainMenuScreen CLASSNAME;

	// Fired with one of the SCREEN_* ids, or Null (String) for Exit.
	Event<String> WhenSelect;

	MainMenuScreen();
};

#endif
