#ifndef _Umbrella_MainMenuScreen_h_
#define _Umbrella_MainMenuScreen_h_

#include <CtrlLib/CtrlLib.h>

using namespace Upp;

class MainMenuScreen : public TopWindow {
private:
	// Main menu controls
	Label titleLabel;
	Button playButton;
	Button levelSelectButton;
	Button editorButton;
	Button quitButton;

	// Level select controls
	Label levelSelectTitle;
	Button backButton;
	Array<Button> levelButtons;

	bool inLevelSelect;

	void ShowMainMenu();
	void ShowLevelSelect();
	void LaunchLevel(const String& path);

public:
	MainMenuScreen();

	void ShowGameScreen();
	void ShowEditorScreen();
};

#endif
