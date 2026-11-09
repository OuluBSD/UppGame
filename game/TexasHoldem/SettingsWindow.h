#ifndef _CardEngine_SettingsWindow_h_
#define _CardEngine_SettingsWindow_h_

#include <CtrlLib/CtrlLib.h>
#include <memory>

NAMESPACE_UPP

#define LAYOUTFILE <TexasHoldem/SettingsWindow.lay>
#include <CtrlCore/lay.h>

class SettingsWindow : public WithSettingsWindowLayout<TopWindow> {
public:
	typedef SettingsWindow CLASSNAME;
	SettingsWindow();
	
	void Init(class ConfigFile& config);

private:
	void OnOK();
	void OnCancel();
	void OnListSelect();

	struct InterfacePage : public WithSettingsInterfaceLayout<ParentCtrl> {
		struct CommonPage : public WithSettingsInterfaceCommonLayout<ParentCtrl> { CommonPage() { CtrlLayout(*this); } } common;
		InterfacePage() { CtrlLayout(*this); tab.Add(common, t_("Common")); }
	} interfacePage;

	struct StylePage : public WithSettingsStyleLayout<ParentCtrl> {
		struct TablePage : public WithSettingsStyleTableLayout<ParentCtrl> { TablePage() { CtrlLayout(*this); } } table;
		StylePage() { CtrlLayout(*this); tab.Add(table, t_("Game Table")); }
	} stylePage;

	struct SoundPage : public WithSettingsSoundLayout<ParentCtrl> { SoundPage() { CtrlLayout(*this); } } soundPage;
	struct LocalGamePage : public WithSettingsLocalGameLayout<ParentCtrl> { LocalGamePage() { CtrlLayout(*this); } } localGamePage;
	struct NetworkGamePage : public WithSettingsNetworkGameLayout<ParentCtrl> { NetworkGamePage() { CtrlLayout(*this); } } networkGamePage;
	
	struct InternetGamePage : public WithSettingsInternetGameLayout<ParentCtrl> {
		struct ServerPage : public WithSettingsInternetServerLayout<ParentCtrl> { ServerPage() { CtrlLayout(*this); } } server;
		InternetGamePage() { CtrlLayout(*this); tab.Add(server, t_("Server")); }
	} internetGamePage;

	struct NicksPage : public WithSettingsNicksLayout<ParentCtrl> { NicksPage() { CtrlLayout(*this); } } nicksPage;
	struct LogPage : public WithSettingsLogLayout<ParentCtrl> { LogPage() { CtrlLayout(*this); } } logPage;
	struct FactoryPage : public WithSettingsFactoryLayout<ParentCtrl> { FactoryPage() { CtrlLayout(*this); } } factoryPage;

	class ConfigFile* m_config;
};

END_UPP_NAMESPACE

#endif
