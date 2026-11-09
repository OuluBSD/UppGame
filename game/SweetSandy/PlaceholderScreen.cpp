#include "SweetSandy.h"

NAMESPACE_UPP

PlaceholderScreen::PlaceholderScreen(const char *title_text)
{
	Add(title.HSizePos(20, 20).TopPos(20, 40));
	title.SetText(title_text).SetFont(StdFont().Bold().Height(32));

	Add(back_button.LeftPos(20, 160).TopPos(70, 32));
	back_button.SetLabel("Takaisin");
	back_button.WhenAction = [=] { WhenBack(); };
}

END_UPP_NAMESPACE
