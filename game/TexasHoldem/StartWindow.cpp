#include "StartWindow.h"
#include "GameTable.h"
#include "LobbyWindow.h"
#include "SettingsWindow.h"
#include <Poker/LocalEngineFactory.h>
#include <GameRules/EngineLog.h>
#include <GameRules/Game.h>
#include <GameRules/PlayerData.h>
#include "ServerManager.h"
#include <EditorCommon/ConfigFile.h>
#include "RunLocalGame.h"
#include <TexasHoldemProviderCatalog/TexasHoldemProviderCatalog.h>

NAMESPACE_UPP

namespace {
constexpr int DEFAULT_LOCAL_PLAYERS = 10;
constexpr int DEFAULT_LOCAL_CASH = 2000;
constexpr int DEFAULT_LOCAL_SPEED = 4;

int StartWindowProviderDefaultPlayers(const String& provider)
{
	const TexasHoldemProviderInfo *info = TexasHoldemFindProvider(provider);
	return info ? info->default_players : DEFAULT_LOCAL_PLAYERS;
}

int StartWindowValidLocalPlayers(int value)
{
	return value >= 2 && value <= 10 ? value : DEFAULT_LOCAL_PLAYERS;
}

int StartWindowValidLocalCash(int value)
{
	return value >= 1000 && value <= 1000000 ? value : DEFAULT_LOCAL_CASH;
}

int StartWindowValidLocalSpeed(int value)
{
	return value >= 1 && value <= 11 ? value : DEFAULT_LOCAL_SPEED;
}
}

static String ResolveStartWindowFormPath()
{
	Vector<String> candidates;
	candidates.Add(AppendFileName(GetCurrentDirectory(), "game/TexasHoldem/StartWindow.form"));
	candidates.Add(AppendFileName(GetFileDirectory(GetExeFilePath()), "../game/TexasHoldem/StartWindow.form"));
	candidates.Add(ConfigFile("StartWindow.form"));
	for (const String& candidate : candidates) {
		if (FileExists(candidate))
			return candidate;
	}
	return ConfigFile("StartWindow.form");
}

static String ResolveNewGameFormPath()
{
	Vector<String> candidates;
	candidates.Add(AppendFileName(GetCurrentDirectory(), "game/TexasHoldem/NewGame.form"));
	candidates.Add(AppendFileName(GetFileDirectory(GetExeFilePath()), "../game/TexasHoldem/NewGame.form"));
	candidates.Add(ConfigFile("NewGame.form"));
	for (const String& candidate : candidates) {
		if (FileExists(candidate))
			return candidate;
	}
	return ConfigFile("NewGame.form");
}

StartWindow::StartWindow()
{
	Sizeable().Zoomable();
	Title("PKR - Texas Hold'em");
	SetRect(GetWorkArea().CenterRect(1024, 720));

	String form_path = ResolveStartWindowFormPath();
	if (!ui.Load(form_path))
		ASSERT_(false, "Could not load StartWindow.form");
	ui.SetScaleMode(Form::SCALE_FIT);
	if (!ui.Layout("Default"))
		ASSERT_(false, "StartWindow.form missing Default layout");
	Add(ui.SizePos());
	ui.SignalHandler = callback(this, &StartWindow::HandleUiSignal);
	ui.Show();

	String setup_path = ResolveNewGameFormPath();
	if (!setup.Load(setup_path))
		ASSERT_(false, "Could not load NewGame.form");
	if (!setup.Layout("Default"))
		ASSERT_(false, "NewGame.form missing Default layout");
	setup.SignalHandler = callback(this, &StartWindow::HandleSetupSignal);
	Add(setup.SizePos());
	setup.Hide();

	numPlayers = dynamic_cast<EditInt*>(setup.GetCtrl("numPlayers"));
	startCash = dynamic_cast<EditInt*>(setup.GetCtrl("startCash"));
	gameSpeed = dynamic_cast<EditInt*>(setup.GetCtrl("gameSpeed"));
	ASSERT(numPlayers && startCash && gameSpeed);

	providerChoice = dynamic_cast<DropList*>(ui.GetCtrl("providerChoice"));
	ASSERT(providerChoice);
	FillProviders();

	table = std::make_unique<GameTable>(SelectedProvider());
	m_tableProvider = SelectedProvider();
	Add(table->SizePos());
	table->Hide();

	m_config = nullptr;
	UpdateTitleFromProvider();
}

StartWindow::~StartWindow() {}

void StartWindow::FillProviders()
{
	ASSERT(providerChoice);
	providerChoice->Clear();
	for(const TexasHoldemProviderInfo& provider : TexasHoldemListProviders())
		providerChoice->Add(provider.id, t_(provider.label));
	providerChoice->SetData(TexasHoldemDefaultProviderId());
	m_provider = AsString(providerChoice->GetData());
}

String StartWindow::SelectedProvider() const
{
	if (providerChoice) {
		String provider = AsString(providerChoice->GetData());
		String canonical = TexasHoldemCanonicalProvider(provider);
		if (!canonical.IsEmpty())
			return canonical;
	}
	return m_provider.IsEmpty() ? TexasHoldemDefaultProviderId() : m_provider;
}

void StartWindow::UpdateTitleFromProvider()
{
	String provider = SelectedProvider();
	m_provider = provider;
	Title(Format("PKR - Texas Hold'em [%s]", provider));
}

void StartWindow::HandleUiSignal(const String& path, const String& op, const String& action)
{
	if (path == "providerChoice") {
		UpdateTitleFromProvider();
		return;
	}
	if (op != "OnAction")
		return;
	if (action == "LocalGame")
		OnLocalGame();
	else if (action == "InternetGame")
		OnInternetGame();
	else if (action == "NetworkGame")
		OnNetworkCreate();
	else if (action == "JoinNetwork")
		OnNetworkJoin();
	else if (action == "Settings")
		OnSettings();
	else if (action == "Log")
		OnLog();
	else if (action == "Info")
		OnInfo();
	else if (action == "Quit")
		OnQuit();
}

void StartWindow::HandleSetupSignal(const String& path, const String& op, const String& action)
{
	if (op != "OnAction")
		return;
	if (action == "Back")
		ShowMenuScreen();
	else if (action == "StartLocalGame")
		StartLocalGameFromSetup();
}

void StartWindow::ShowMenuScreen()
{
	if (table)
		table->Hide();
	setup.Hide();
	ui.Show();
	Title(Format("PKR - Texas Hold'em [%s]", SelectedProvider()));
}

void StartWindow::ShowSetupScreen()
{
	LoadSetupDefaults();
	if (table)
		table->Hide();
	ui.Hide();
	setup.Show();
	Title(t_("PKR - New Local Game"));
}

void StartWindow::ShowGameScreen()
{
	EnsureTableForProvider();
	ui.Hide();
	setup.Hide();
	table->Show();
	table->RefreshLayoutDeep();
	Title(t_("PKR - Texas Hold'em"));
}

void StartWindow::LoadSetupDefaults()
{
	int provider_players = StartWindowProviderDefaultPlayers(SelectedProvider());
	if (!m_config) {
		numPlayers->SetData(provider_players);
		startCash->SetData(DEFAULT_LOCAL_CASH);
		gameSpeed->SetData(DEFAULT_LOCAL_SPEED);
		return;
	}
	if (provider_players != DEFAULT_LOCAL_PLAYERS)
		numPlayers->SetData(provider_players);
	else {
		int stored_players = m_config->readConfigInt("LocalNumPlayers");
		numPlayers->SetData(StartWindowValidLocalPlayers(stored_players > 0 ? stored_players : DEFAULT_LOCAL_PLAYERS));
	}
	startCash->SetData(StartWindowValidLocalCash(m_config->readConfigInt("LocalStartCash")));
	gameSpeed->SetData(StartWindowValidLocalSpeed(m_config->readConfigInt("LocalGameSpeed")));
}

void StartWindow::StartLocalGameFromSetup()
{
	if (!m_engineLog || !m_config)
		return;
	StartLocalGameWithValues((int)numPlayers->GetData(), (int)startCash->GetData(),
	                         (int)gameSpeed->GetData());
}

void StartWindow::StartLocalGameForTest(int players, int cash, int speed)
{
	StartLocalGameWithValues(players, cash, speed);
}

void StartWindow::SelectProviderForTest(const String& provider)
{
	String canonical = TexasHoldemCanonicalProvider(provider);
	if(canonical.IsEmpty())
		canonical = TexasHoldemDefaultProviderId();
	if (providerChoice)
		providerChoice->SetData(canonical);
	UpdateTitleFromProvider();
}

void StartWindow::ShowSetupForTest()
{
	ShowSetupScreen();
}

void StartWindow::StartLocalGameWithValues(int players, int cash, int speed)
{
	if (!m_engineLog || !m_config)
		return;
	players = StartWindowValidLocalPlayers(players);
	cash = StartWindowValidLocalCash(cash);
	speed = StartWindowValidLocalSpeed(speed);
	ShowGameScreen();
	InitLocalGame(*table, players, cash, speed, SelectedProvider(), *m_config, *m_engineLog);
	table->RefreshLayoutDeep();
	table->Refresh();
}

void StartWindow::EnsureTableForProvider()
{
	String provider = SelectedProvider();
	if (table && provider == m_tableProvider)
		return;
	if (table)
		table->Remove();
	table = std::make_unique<GameTable>(provider);
	m_tableProvider = provider;
	Add(table->SizePos());
	table->Hide();
}

void StartWindow::DumpSetupState(Stream& out) const
{
	out << "setup players=" << (int)numPlayers->GetData()
	    << " cash=" << (int)startCash->GetData()
	    << " speed=" << (int)gameSpeed->GetData()
	    << " setup_shown=" << (setup.IsShown() ? 1 : 0) << "\n";
	out.Flush();
}

void StartWindow::DumpEmbeddedGameState(Stream& out) const
{
	table->DumpLayoutRects(out);
	table->DumpGameState(out);
}

void StartWindow::OnLocalGame()
{
	UpdateTitleFromProvider();
	ShowSetupScreen();
}

void StartWindow::OnInternetGame()
{
	LobbyWindow().Run();
}

void StartWindow::OnNetworkCreate()
{
	// TODO: Create network game
}

void StartWindow::OnNetworkJoin()
{
	LobbyWindow().Run();
}

void StartWindow::OnInfo()
{
	PromptOK(Format("PKR U++ Port v0.1\nProvider: %s\nOriginal-style local card table.", SelectedProvider()));
}

void StartWindow::OnSettings()
{
	SettingsWindow dlg;
	if (m_config) dlg.Init(*m_config);
	dlg.Run();
}

void StartWindow::OnLog()
{
	PromptOK(t_("Log window not implemented yet."));
}

void StartWindow::OnQuit()
{
	Break();
}

END_UPP_NAMESPACE
