#include "Session.h"
#include <GameRules/Game.h>
#include <EditorCommon/ConfigFile.h>
#include <GameRules/EngineLog.h>
#include "ClientThread.h"
#include "ServerManager.h"
#include "AvatarManager.h"

NAMESPACE_UPP

Session::Session(GuiInterface* gui, class ConfigFile* config, EngineLog* log)
	: m_gui(gui), m_config(config), m_log(log)
{
}

Session::~Session()
{
}

bool Session::Init()
{
	m_avatarManager = std::make_shared<AvatarManager>();
	return true;
}

void Session::Init(std::shared_ptr<AvatarManager> manager)
{
	m_avatarManager = manager;
}

void Session::StartLocalGame(const GameData &gameData, const StartData &startData)
{
	m_gameType = GAME_TYPE_LOCAL;
}

void Session::StartClientGame(std::shared_ptr<Game> game)
{
	m_currentGame = game;
}

std::shared_ptr<Game> Session::GetCurrentGame()
{
	return m_currentGame;
}

void Session::StartInternetClient()
{
	m_gameType = GAME_TYPE_INTERNET;
}

void Session::StartNetworkClient(const String &serverAddress, unsigned serverPort, bool ipv6, bool sctp)
{
	m_gameType = GAME_TYPE_NETWORK;
}

void Session::TerminateNetworkClient()
{
	if (m_netClient) {
		m_netClient->SignalTermination();
		m_netClient.reset();
	}
}

void Session::StartNetworkServer(bool dedicated)
{
}

void Session::TerminateNetworkServer()
{
	if (m_netServer) {
		m_netServer->SignalTerminationAll();
		m_netServer.reset();
	}
}

bool Session::IsNetworkClientRunning() const { return m_netClient != nullptr; }
bool Session::IsNetworkServerRunning() const { return m_netServer != nullptr; }

END_UPP_NAMESPACE
