#include "SettingsWindow.h"
#include <EditorCommon/ConfigFile.h>
#include <GameRules/GameDefs.h>
#include <EditorCommon/Tools.h>

NAMESPACE_UPP

SettingsWindow::SettingsWindow()
{
	CtrlLayout(*this, t_("Settings"));
	
	list.AddColumn(t_("Category"));
	list.NoHeader();
	list.Add(t_("Interface"));
	list.Add(t_("Style"));
	list.Add(t_("Sound"));
	list.Add(t_("Local Game"));
	list.Add(t_("Network Game"));
	list.Add(t_("Internet Game"));
	list.Add(t_("Nicks/Avatars"));
	list.Add(t_("Log Messages"));
	list.Add(t_("Factory Settings"));
	
	pages.Add(interfacePage.SizePos());
	pages.Add(stylePage.SizePos());
	pages.Add(soundPage.SizePos());
	pages.Add(localGamePage.SizePos());
	pages.Add(networkGamePage.SizePos());
	pages.Add(internetGamePage.SizePos());
	pages.Add(nicksPage.SizePos());
	pages.Add(logPage.SizePos());
	pages.Add(factoryPage.SizePos());
	
	interfacePage.lblTitle.SetFont(StdFont().Bold());
	stylePage.lblTitle.SetFont(StdFont().Bold());
	soundPage.lblTitle.SetFont(StdFont().Bold());
	localGamePage.lblTitle.SetFont(StdFont().Bold());
	networkGamePage.lblTitle.SetFont(StdFont().Bold());
	internetGamePage.lblTitle.SetFont(StdFont().Bold());
	nicksPage.lblTitle.SetFont(StdFont().Bold());
	logPage.lblTitle.SetFont(StdFont().Bold());
	factoryPage.lblTitle.SetFont(StdFont().Bold());

	auto& lang = interfacePage.common.language;
	lang.Add("en-us", "English (US)");
	lang.Add("de-de", "Deutsch");
	lang.Add("fr-fr", "Français");
	lang.Add("fi-fi", "Suomi");
	
	auto& app = interfacePage.common.appearance;
	app.Add("auto", t_("Auto (System)"));
	app.Add("light", t_("Light"));
	app.Add("dark", t_("Dark"));

	soundPage.volume.Range(10);

	stylePage.table.tableStyles.AddColumn(t_("Name"));
	
	String themeDir = AppendFileName(Tools::GetDataDir(), "gfx/gui/table");
	FindFile ff(AppendFileName(themeDir, "*.*"));
	while(ff) {
		if(ff.IsFolder() && ff.GetName() != "." && ff.GetName() != "..") {
			stylePage.table.tableStyles.Add(ff.GetName());
		}
		ff.Next();
	}
	
	stylePage.table.tableStyles.WhenSel = [this, themeDir] {
		if (stylePage.table.tableStyles.IsCursor()) {
			String theme = stylePage.table.tableStyles.Get(0);
			String path = AppendFileName(themeDir, theme + "/preview.png");
			stylePage.table.preview.SetImage(StreamRaster::LoadFileAny(path));
		}
	};

	String cardDir = AppendFileName(Tools::GetDataDir(), "gfx/cards");
	FindFile ffCard(AppendFileName(cardDir, "*.*"));
	while(ffCard) {
		if(ffCard.IsFolder() && ffCard.GetName() != "." && ffCard.GetName() != "..") {
			stylePage.table.cardTheme.Add(ffCard.GetName());
		}
		ffCard.Next();
	}

	nicksPage.humanAvatar << [this] {
		FileSel fs;
		fs.Type(t_("Images"), "*.png *.jpg *.gif");
		if (fs.ExecuteOpen(t_("Select Avatar"))) {
			nicksPage.humanAvatar.SetImage(StreamRaster::LoadFileAny(~fs));
		}
	};

	list.WhenSel = [this] { OnListSelect(); };
	list.SetCursor(0);
	OnListSelect();

	btnOK << [this] { OnOK(); };
	btnCancel << [this] { OnCancel(); };
	
	factoryPage.btnReset << [this] {
		if (PromptYesNo(t_("Do you really want to reset all settings to default?"))) {
			if (m_config) {
				m_config->clearBuffer();
				Init(*m_config);
			}
		}
	};
	
	m_config = nullptr;
}

void SettingsWindow::OnListSelect()
{
	int c = list.GetCursor();
	interfacePage.Show(c == 0);
	stylePage.Show(c == 1);
	soundPage.Show(c == 2);
	localGamePage.Show(c == 3);
	networkGamePage.Show(c == 4);
	internetGamePage.Show(c == 5);
	nicksPage.Show(c == 6);
	logPage.Show(c == 7);
	factoryPage.Show(c == 8);
}

template <class T>
void SetConfigData(T& ctrl, class ConfigFile* cfg, const char* key, const Value& def)
{
	if (!cfg) {
		ctrl.SetData(def);
		return;
	}
	String s = cfg->readConfigString(key);
	if (s.IsEmpty()) {
		ctrl.SetData(def);
	}
	else {
		if (def.Is<int>())
			ctrl.SetData(StrInt(s));
		else
			ctrl.SetData(s);
	}
}

void SettingsWindow::Init(class ConfigFile& config)
{
	m_config = &config;
	
	// Interface
	SetConfigData(interfacePage.common.language, m_config, "Language", "en-us");
	SetConfigData(interfacePage.common.appearance, m_config, "Appearance", "auto");
	SetConfigData(interfacePage.common.showRightToolbox, m_config, "ShowRightToolbox", 1);
	SetConfigData(interfacePage.common.showLeftToolbox, m_config, "ShowLeftToolbox", 1);
	SetConfigData(interfacePage.common.showFadeOut, m_config, "ShowFadeOut", 1);
	SetConfigData(interfacePage.common.showFlip, m_config, "ShowFlip", 1);
	SetConfigData(interfacePage.common.reverseFKeys, m_config, "ReverseFKeys", 0);
	SetConfigData(interfacePage.common.showBlindButtons, m_config, "ShowBlindButtons", 1);
	SetConfigData(interfacePage.common.cardsChanceMonitor, m_config, "CardsChanceMonitor", 1);
	SetConfigData(interfacePage.common.antiPeekMode, m_config, "AntiPeekMode", 0);
	SetConfigData(interfacePage.common.disableSplash, m_config, "DisableSplash", 0);
	SetConfigData(interfacePage.common.dontTranslate, m_config, "DontTranslate", 0);
	SetConfigData(interfacePage.common.showPotPercent, m_config, "ShowPotPercent", 1);
	
	// Sound
	SetConfigData(soundPage.playSounds, m_config, "PlaySounds", 1);
	SetConfigData(soundPage.volume, m_config, "SoundVolume", 8);
	SetConfigData(soundPage.gameActions, m_config, "SoundGameActions", 1);
	SetConfigData(soundPage.lobbyChat, m_config, "SoundLobbyChat", 1);
	SetConfigData(soundPage.netNotifications, m_config, "SoundNetNotifications", 1);
	SetConfigData(soundPage.blindRaise, m_config, "SoundBlindRaise", 1);
	
	// Local Game
	SetConfigData(localGamePage.numPlayers, m_config, "LocalNumPlayers", 10);
	SetConfigData(localGamePage.startCash, m_config, "LocalStartCash", 1500);
	SetConfigData(localGamePage.firstBlind, m_config, "LocalFirstSmallBlind", 10);
	
	// Network
	SetConfigData(networkGamePage.serverPort, m_config, "ServerPort", 7234);
	SetConfigData(networkGamePage.useIpv6, m_config, "UseIpv6", 0);
	SetConfigData(networkGamePage.useSctp, m_config, "UseSctp", 0);

	// Internet
	SetConfigData(internetGamePage.server.serverList, m_config, "ServerListAddress", "");
	
	// Nicks
	SetConfigData(nicksPage.humanNick, m_config, "Nick", "Player");

	// Log
	SetConfigData(logPage.enableLogging, m_config, "EnableLogging", 0);
	SetConfigData(logPage.logDuration, m_config, "LogDuration", 7);
	
	// Theme
	String theme = m_config->readConfigString("TableTheme");
	if(theme.IsEmpty()) theme = "default";
	for(int i = 0; i < stylePage.table.tableStyles.GetCount(); i++) {
		if(stylePage.table.tableStyles.Get(i, 0).ToString() == theme) {
			stylePage.table.tableStyles.SetCursor(i);
			break;
		}
	}
	
	SetConfigData(stylePage.table.cardTheme, m_config, "CardTheme", "default_800x480");
}

void SettingsWindow::OnOK()
{
	if (m_config) {
		// Interface
		m_config->writeConfigString("Language", interfacePage.common.language.GetData());
		m_config->writeConfigString("Appearance", interfacePage.common.appearance.GetData());
		m_config->writeConfigInt("ShowRightToolbox", interfacePage.common.showRightToolbox.GetData());
		m_config->writeConfigInt("ShowLeftToolbox", interfacePage.common.showLeftToolbox.GetData());
		m_config->writeConfigInt("ShowFadeOut", interfacePage.common.showFadeOut.GetData());
		m_config->writeConfigInt("ShowFlip", interfacePage.common.showFlip.GetData());
		m_config->writeConfigInt("ReverseFKeys", interfacePage.common.reverseFKeys.GetData());
		m_config->writeConfigInt("ShowBlindButtons", interfacePage.common.showBlindButtons.GetData());
		m_config->writeConfigInt("CardsChanceMonitor", interfacePage.common.cardsChanceMonitor.GetData());
		m_config->writeConfigInt("AntiPeekMode", interfacePage.common.antiPeekMode.GetData());
		m_config->writeConfigInt("DisableSplash", interfacePage.common.disableSplash.GetData());
		m_config->writeConfigInt("DontTranslate", interfacePage.common.dontTranslate.GetData());
		m_config->writeConfigInt("ShowPotPercent", interfacePage.common.showPotPercent.GetData());

		// Style
		if(stylePage.table.tableStyles.IsCursor())
			m_config->writeConfigString("TableTheme", stylePage.table.tableStyles.Get(0));
		m_config->writeConfigString("CardTheme", stylePage.table.cardTheme.GetData().ToString());

		// Sound
		m_config->writeConfigInt("PlaySounds", soundPage.playSounds.GetData());
		m_config->writeConfigInt("SoundVolume", soundPage.volume.GetData());
		m_config->writeConfigInt("SoundGameActions", soundPage.gameActions.GetData());
		m_config->writeConfigInt("SoundLobbyChat", soundPage.lobbyChat.GetData());
		m_config->writeConfigInt("SoundNetNotifications", soundPage.netNotifications.GetData());
		m_config->writeConfigInt("SoundBlindRaise", soundPage.blindRaise.GetData());

		// Local Game
		m_config->writeConfigInt("LocalNumPlayers", localGamePage.numPlayers.GetData());
		m_config->writeConfigInt("LocalStartCash", localGamePage.startCash.GetData());
		m_config->writeConfigInt("LocalFirstSmallBlind", localGamePage.firstBlind.GetData());

		// Network
		m_config->writeConfigInt("ServerPort", networkGamePage.serverPort.GetData());
		m_config->writeConfigInt("UseIpv6", networkGamePage.useIpv6.GetData());
		m_config->writeConfigInt("UseSctp", networkGamePage.useSctp.GetData());

		// Internet
		m_config->writeConfigString("ServerListAddress", internetGamePage.server.serverList.GetData());

		// Nicks
		m_config->writeConfigString("Nick", nicksPage.humanNick.GetData());

		// Log
		m_config->writeConfigInt("EnableLogging", logPage.enableLogging.GetData());
		m_config->writeConfigInt("LogDuration", logPage.logDuration.GetData());
		
		m_config->writeBuffer();
	}
	Break(IDOK);
}

void SettingsWindow::OnCancel()
{
	Break(IDCANCEL);
}

END_UPP_NAMESPACE
