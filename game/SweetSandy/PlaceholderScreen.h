#ifndef _SweetSandy_PlaceholderScreen_h_
#define _SweetSandy_PlaceholderScreen_h_

// Reusable stand-in for a screen that has no content yet (Settings,
// Multiplayer, Credits, Tutorial) and for screens whose real content lands
// in a later task (New game, Play) - just a title and a back button.
class PlaceholderScreen : public ParentCtrl {
	Label  title;
	Button back_button;

public:
	typedef PlaceholderScreen CLASSNAME;

	Event<> WhenBack;

	PlaceholderScreen(const char *title_text);
};

#endif
