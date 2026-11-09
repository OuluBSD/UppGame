#ifndef _CardEngine_StartWindow_h_
#define _CardEngine_StartWindow_h_

#include <CtrlLib/CtrlLib.h>
#include <Form/Form.hpp>
#include <memory>

NAMESPACE_UPP

class StartWindow : public TopWindow {
public:
	typedef StartWindow CLASSNAME;
	StartWindow();
	~StartWindow();

	void Init(class ConfigFile& config, std::shared_ptr<class EngineLog> engineLog) {
		m_config = &config;
		m_engineLog = engineLog;
	}
	void StartLocalGameForTest(int players, int cash, int speed);
	void SelectProviderForTest(const String& provider);
	void ShowSetupForTest();
	void DumpSetupState(Stream& out) const;
	void DumpEmbeddedGameState(Stream& out) const;

private:
	void HandleUiSignal(const String& path, const String& op, const String& action);
	void HandleSetupSignal(const String& path, const String& op, const String& action);
	void FillProviders();
	void UpdateTitleFromProvider();
	String SelectedProvider() const;
	void ShowMenuScreen();
	void ShowSetupScreen();
	void ShowGameScreen();
	void LoadSetupDefaults();
	void StartLocalGameFromSetup();
	void StartLocalGameWithValues(int players, int cash, int speed);
	void EnsureTableForProvider();
	void OnLocalGame();
	void OnInternetGame();
	void OnNetworkCreate();
	void OnNetworkJoin();
	void OnInfo();
	void OnSettings();
	void OnLog();
	void OnQuit();

	Form ui;
	Form setup;
	DropList* providerChoice = nullptr;
	EditInt* numPlayers = nullptr;
	EditInt* startCash = nullptr;
	EditInt* gameSpeed = nullptr;
	std::unique_ptr<class GameTable> table;
	String m_tableProvider;
	class ConfigFile* m_config;
	std::shared_ptr<class ServerManager> m_serverManager;
	std::shared_ptr<class EngineLog> m_engineLog;
	String m_provider;
};
END_UPP_NAMESPACE

#endif
