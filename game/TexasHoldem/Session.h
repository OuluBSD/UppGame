#ifndef _CardEngine_Session_h_
#define _CardEngine_Session_h_

#include <Core/Core.h>
#include <memory>
#include "ServerData.h"
#include <GameRules/GameData.h>
#include <GameRules/PlayerData.h>
#include <GameRules/GameDefs.h>
#include <EditorCommon/CryptHelper.h>

NAMESPACE_UPP

class GuiInterface;
class Game;
class ConfigFile;
class EngineLog;
class ClientThread;
class ServerManager;
class AvatarManager;

class Session
{
public:
	Session(GuiInterface* gui, class ConfigFile* config, EngineLog* log);
	~Session();

	enum GameType { GAME_TYPE_NONE, GAME_TYPE_LOCAL, GAME_TYPE_NETWORK, GAME_TYPE_INTERNET };

	bool Init();
	void Init(std::shared_ptr<AvatarManager> manager);

	void StartLocalGame(const GameData &gameData, const StartData &startData);
	void StartClientGame(std::shared_ptr<Game> game);

	std::shared_ptr<Game> GetCurrentGame();

	GuiInterface* GetGui() { return m_gui; }
	EngineLog* GetMyLog() { return m_log; }
	GameType GetGameType() { return m_gameType; }

	std::shared_ptr<AvatarManager> GetAvatarManager() { return m_avatarManager; }

	void StartInternetClient();
	void StartNetworkClient(const String &serverAddress, unsigned serverPort, bool ipv6, bool sctp);
	void TerminateNetworkClient();
	
	void StartNetworkServer(bool dedicated);
	void TerminateNetworkServer();

	bool IsNetworkClientRunning() const;
	bool IsNetworkServerRunning() const;

private:
	int m_currentGameNum = 1;
	std::shared_ptr<ClientThread> m_netClient;
	std::shared_ptr<ServerManager> m_netServer;
	std::shared_ptr<AvatarManager> m_avatarManager;
	std::shared_ptr<Game> m_currentGame;
	
	GuiInterface *m_gui;
	class ConfigFile *m_config;
	EngineLog *m_log;
	GameType m_gameType = GAME_TYPE_NONE;
};

END_UPP_NAMESPACE

#endif
