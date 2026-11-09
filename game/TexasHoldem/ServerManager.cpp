#include "ServerManager.h"
#include "ServerLobbyThread.h"
#include <EditorCommon/ConfigFile.h>

NAMESPACE_UPP

ServerManager::ServerManager(class ConfigFile &config, GuiInterface &gui)
	: m_playerConfig(config), m_gui(gui)
{
}

ServerManager::ServerManager(class ConfigFile &config, GuiInterface &gui, ServerMode mode, AvatarManager &avatarManager)
	: m_playerConfig(config), m_gui(gui)
{
	m_lobbyThread.reset(new ServerLobbyThread(gui, mode, config, avatarManager));
}

ServerManager::~ServerManager()
{
}

void ServerManager::Init(unsigned serverPort, unsigned websocketPort, bool ipv6, bool serverTls, bool websocketTls, int proto, const String &logDir,
					  const String &webSocketResource, const String &webSocketOrigin)
{
	if (m_lobbyThread) {
		m_lobbyThread->Init(logDir);
	}
	// TODO: Start acceptor thread for serverPort
}

void ServerManager::RunAll()
{
	if (m_lobbyThread) {
		m_lobbyThread->Run();
	}
}

void ServerManager::SignalTerminationAll()
{
	if (m_lobbyThread) {
		m_lobbyThread->SignalTermination();
	}
}

bool ServerManager::JoinAll(bool wait)
{
	if (m_lobbyThread) {
		return m_lobbyThread->Join(wait ? 20000 : 0);
	}
	return true;
}

ServerLobbyThread & ServerManager::GetLobbyThread()
{
	return *m_lobbyThread;
}

END_UPP_NAMESPACE