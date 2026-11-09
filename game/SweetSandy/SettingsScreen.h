#ifndef _SweetSandy_SettingsScreen_h_
#define _SweetSandy_SettingsScreen_h_

// Rebindable driving keys - click a row's button, then press the key you
// want; Esc while capturing cancels instead of binding Esc itself.
class SettingsScreen : public ParentCtrl {
	Label  title;
	Label  action_label[5];
	Button key_button[5];
	Button back_button;

	int capturing = -1; // index into action_label/key_button, -1 = not capturing

	static dword* BindingField(int i);
	static const char *ActionName(int i);
	void RefreshRow(int i);
	void RefreshAll();
	void StartCapture(int i);

public:
	typedef SettingsScreen CLASSNAME;

	Event<> WhenBack;

	SettingsScreen();
	bool Key(dword key, int count) override;
};

#endif
