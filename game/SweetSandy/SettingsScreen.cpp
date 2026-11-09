#include "SweetSandy.h"

NAMESPACE_UPP

dword* SettingsScreen::BindingField(int i)
{
	KeyBindings& k = GetKeyBindings();
	switch(i) {
	case 0: return &k.throttle;
	case 1: return &k.brake;
	case 2: return &k.steer_left;
	case 3: return &k.steer_right;
	default: return &k.back;
	}
}

const char *SettingsScreen::ActionName(int i)
{
	switch(i) {
	case 0: return "Kaasu";
	case 1: return "Jarru";
	case 2: return "Ohjaa vasemmalle";
	case 3: return "Ohjaa oikealle";
	default: return "Takaisin valikkoon";
	}
}

SettingsScreen::SettingsScreen()
{
	WantFocus();

	Add(title.HSizePos(20, 20).TopPos(20, 40));
	title.SetText("Asetukset").SetFont(StdFont().Bold().Height(32));

	int y = 90;
	int step = 40;
	for(int i = 0; i < 5; i++) {
		Add(action_label[i].LeftPos(20, 220).TopPos(y, 28));
		action_label[i].SetText(ActionName(i));
		Add(key_button[i].LeftPos(260, 160).TopPos(y, 28));
		key_button[i].WhenAction = [=] { StartCapture(i); };
		y += step;
	}

	Add(back_button.LeftPos(20, 160).TopPos(y + 20, 32));
	back_button.SetLabel("Takaisin");
	back_button.WhenAction = [=] { WhenBack(); };

	RefreshAll();
}

void SettingsScreen::RefreshRow(int i)
{
	if(i == capturing)
		key_button[i].SetLabel("(paina nappainta...)");
	else
		key_button[i].SetLabel(KeyName(*BindingField(i)));
}

void SettingsScreen::RefreshAll()
{
	for(int i = 0; i < 5; i++)
		RefreshRow(i);
}

void SettingsScreen::StartCapture(int i)
{
	capturing = i;
	RefreshAll();
	SetFocus(); // move focus off the button so the next Key() reaches us, not it
}

bool SettingsScreen::Key(dword key, int count)
{
	if(capturing >= 0 && !(key & K_KEYUP)) {
		int i = capturing;
		capturing = -1;
		if(key != K_ESCAPE) { // Esc cancels the capture instead of binding itself
			*BindingField(i) = key;
			SaveKeyBindings();
		}
		RefreshAll();
		return true;
	}
	return ParentCtrl::Key(key, count);
}

END_UPP_NAMESPACE
