#ifndef _CardEngine_ServerLobbyThread_h_
#define _CardEngine_ServerLobbyThread_h_

#include <Core/Core.h>
#include "Thread.h"
#include "SessionManager.h"
#include "ServerData.h"
#include <GameRules/GameData.h>
#include <GameRules/GuiInterface.h>
#include <GameRules/EngineFactory.h>
#include <GameRules/EngineLog.h>
#include "SenderHelper.h"

NAMESPACE_UPP

class ServerGame;
class ConfigFile;
class AvatarManager;
class ServerDBInterface;
class NetPacket;

class ServerLobbyThread : public ThreadWrapper, public std::enable_shared_from_this<ServerLobbyThread>
{
public:
	ServerLobbyThread(GuiInterface &gui, ServerMode mode, class ConfigFile &serverConfig, AvatarManager &avatarManager);
	virtual ~ServerLobbyThread();

	void Init(const String &logDir);
	virtual void SignalTermination() override;

	void AddConnection(std::shared_ptr<SessionData> sessionData);
	void ReAddSession(std::shared_ptr<SessionData> session, int reason, unsigned gameId);
	void MoveSessionToGame(std::shared_ptr<ServerGame> game, std::shared_ptr<SessionData> session, bool autoLeave, bool spectateOnly);
	void SessionError(std::shared_ptr<SessionData> session, int errorCode);
	
	void DispatchPacket(std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet);

	SenderHelper& GetSender() { return *m_sender; }

	uint32 GetNextSessionId();
	std::shared_ptr<EngineFactory> GetEngineFactory() { return m_engineFactory; }
	EngineLog& GetLog() { return *m_log; }
	SessionDataCallback& GetSessionDataCallback();
	uint32 GetNextUniquePlayerId();
	uint32 GetNextGameId();

	ServerStats GetStats() const;\
	ServerMode GetServerMode() const;

protected:
	virtual void Main() override;
	void RegisterTimers();
	void CancelTimers();

	void HandlePacket(std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet);
	
	void CloseSession(std::shared_ptr<SessionData> session);
	void UpdateStatisticsNumberOfPlayers();

private:
	GuiInterface &m_gui;
	ServerMode m_mode;
	class ConfigFile &m_serverConfig;
	AvatarManager &m_avatarManager;

	SessionManager m_sessionManager;
	SessionManager m_gameSessionManager;

	VectorMap<unsigned, std::shared_ptr<ServerGame>> m_gameMap;
	
	uint32 m_curGameId = 0;
	uint32 m_curUniquePlayerId = 0;
	uint32 m_curSessionId = 1;
	
	ServerStats m_statData;
	mutable Mutex m_statMutex;

	std::shared_ptr<SenderHelper> m_sender;
	std::shared_ptr<ServerDBInterface> m_database;
	std::shared_ptr<EngineFactory> m_engineFactory;
	std::shared_ptr<EngineLog> m_log;
	
	String m_statisticsFileName;
};

END_UPP_NAMESPACE

#endif
