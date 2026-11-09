#include "SweetSandy.h"

NAMESPACE_UPP

MainMenuScreen::MainMenuScreen()
{
	Add(title.HSizePos(20, 20).TopPos(20, 60));
	title.SetText("SweetSandy").SetFont(StdFont().Bold().Height(48));

	int y = 100;
	int step = 44;
	Add(tutorial_button.LeftPos(20, 220).TopPos(y += 0 * step, 36));
	Add(newgame_button.LeftPos(20, 220).TopPos(y += step, 36));
	Add(settings_button.LeftPos(20, 220).TopPos(y += step, 36));
	Add(multiplayer_button.LeftPos(20, 220).TopPos(y += step, 36));
	Add(credits_button.LeftPos(20, 220).TopPos(y += step, 36));
	Add(exit_button.LeftPos(20, 220).TopPos(y += step, 36));

	tutorial_button.SetLabel("Tutorial");
	newgame_button.SetLabel("Uusi peli");
	settings_button.SetLabel("Asetukset");
	multiplayer_button.SetLabel("Moninpeli");
	credits_button.SetLabel("Credits");
	exit_button.SetLabel("Exit");

	tutorial_button.WhenAction = [=] { WhenSelect(SCREEN_TUTORIAL); };
	newgame_button.WhenAction = [=] { WhenSelect(SCREEN_NEWGAME); };
	settings_button.WhenAction = [=] { WhenSelect(SCREEN_SETTINGS); };
	multiplayer_button.WhenAction = [=] { WhenSelect(SCREEN_MULTIPLAYER); };
	credits_button.WhenAction = [=] { WhenSelect(SCREEN_CREDITS); };
	exit_button.WhenAction = [=] { WhenSelect(Null); };
}

END_UPP_NAMESPACE
