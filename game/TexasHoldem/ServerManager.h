#ifndef _CardEngine_ServerManager_h_
#define _CardEngine_ServerManager_h_

#include <Core/Core.h>
#include <GameRules/GameDefs.h>
#include <GameRules/GuiInterface.h>

NAMESPACE_UPP

class ServerLobbyThread;
class ConfigFile;
class AvatarManager;

class ServerManager
{
public:
	ServerManager(class ConfigFile &config, GuiInterface &gui);
	ServerManager(class ConfigFile &config, GuiInterface &gui, ServerMode mode, AvatarManager &avatarManager);
	virtual ~ServerManager();

	virtual void Init(unsigned serverPort, unsigned websocketPort, bool ipv6, bool serverTls, bool websocketTls, int proto, const String &logDir,
					  const String &webSocketResource, const String &webSocketOrigin);

	virtual void RunAll();
	virtual void SignalTerminationAll();
	virtual bool JoinAll(bool wait);

	ServerLobbyThread &GetLobbyThread();

protected:
	std::shared_ptr<ServerLobbyThread> m_lobbyThread;

private:
	class ConfigFile &m_playerConfig;
	GuiInterface &m_gui;
};

END_UPP_NAMESPACE

#endif