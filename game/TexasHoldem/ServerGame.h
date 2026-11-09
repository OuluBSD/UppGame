#ifndef _CardEngine_ServerGame_h_
#define _CardEngine_ServerGame_h_

#include <Core/Core.h>
#include "SessionManager.h"
#include "ServerData.h"
#include <GameRules/GameData.h>
#include <GameRules/PlayerData.h>
#include "ServerGameState.h"
#include <GameRules/GuiInterface.h>

NAMESPACE_UPP

class ServerLobbyThread;
class ServerDBInterface;
class Game;
class NetPacket;

struct RankingData {
	RankingData() : dbid(DB_ID_INVALID), place(0) {}
	RankingData(unsigned d) : dbid(d), place(0) {}
	unsigned dbid;
	int place;
};

class ServerGame : public std::enable_shared_from_this<ServerGame>
{
public:
	ServerGame(std::shared_ptr<ServerLobbyThread> lobbyThread, uint32 id, const String &name, const String &pwd, const GameData &gameData,
			   unsigned adminPlayerId, unsigned creatorPlayerDBId, GuiInterface &gui, class ConfigFile &playerConfig);
	virtual ~ServerGame();

	void Init();
	void Exit();

	uint32 GetId() const;
	const String& GetName() const;
	unsigned GetCreatorDBId() const;

	void AddSession(std::shared_ptr<SessionData> session, bool spectateOnly);
	void RemovePlayer(unsigned playerId, unsigned errorCode);
	void MutePlayer(unsigned playerId, bool mute);
	void MarkPlayerAsInactive(unsigned playerId);
	void MarkPlayerAsKicked(unsigned playerId);

	void HandlePacket(std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet);

	TexasRound GetCurRound() const;

	void SendToAllPlayers(std::shared_ptr<NetPacket> packet, int state);
	void SendToAllButOnePlayers(std::shared_ptr<NetPacket> packet, SessionId except, int state);
	void RemoveAllSessions();
	void MoveSpectatorsToLobby();

	bool IsPasswordProtected() const;
	bool CheckPassword(const String &password) const;
	static bool CheckSettings(const GameData &data, const String &password, ServerMode mode);

	std::shared_ptr<PlayerData> GetPlayerDataByUniqueId(unsigned playerId) const;
	PlayerIdList GetPlayerIdList() const;
	PlayerIdList GetSpectatorIdList() const;
	bool IsPlayerConnected(const String &name) const;
	bool IsPlayerConnected(unsigned playerId) const;
	bool IsClientAddressConnected(const String &clientAddress) const;
	std::shared_ptr<PlayerInterface> GetPlayerInterfaceFromGame(const String &playerName);
	std::shared_ptr<PlayerInterface> GetPlayerInterfaceFromGame(unsigned playerId);

	bool IsRunning() const;
	void StartGame(const PlayerDataList &playerDataList);

	unsigned GetAdminPlayerId() const;
	void SetAdminPlayerId(unsigned playerId);

	void AddPlayerInvitation(unsigned playerId);
	void RemovePlayerInvitation(unsigned playerId);
	bool IsPlayerInvited(unsigned playerId) const;

	void SetPlayerAutoLeaveOnFinish(unsigned playerId);
	
	void SessionError(std::shared_ptr<SessionData> session, int errorCode);
	void RemoveSession(std::shared_ptr<SessionData> session, int reason);
	void MoveSessionToLobby(std::shared_ptr<SessionData> session, int reason);

	void SetNameReported();
	bool IsNameReported() const;

	SessionManager& GetSessionManager();
	const SessionManager& GetSessionManager() const;
	ServerLobbyThread& GetLobbyThread();
	GuiInterface& GetGui();
	ServerGameState& GetState();
	void SetState(ServerGameState &newState);
	
	Game& GetGame();
	const Game& GetGame() const;
	const GameData& GetGameData() const;
	const StartData& GetStartData() const;

private:
	typedef VectorMap<unsigned, RankingData> RankingMap;
	typedef VectorMap<String, int> NumJoinsPerPlayerMap;

	unsigned m_adminPlayerId;
	std::shared_ptr<ServerLobbyThread> m_lobbyThread;
	GuiInterface &m_gui;
	GameData m_gameData;
	StartData m_startData;
	ServerGameState *m_curState;
	uint32 m_id;
	String m_name;
	String m_password;
	unsigned m_creatorPlayerDBId;
	class ConfigFile &m_playerConfig;
	unsigned m_gameNum;
	unsigned m_curPetitionId;
	bool m_isNameReported;

	SessionManager m_sessionManager;
	std::shared_ptr<ServerDBInterface> m_database;
	std::shared_ptr<Game> m_game;

	RankingMap m_rankingMap;
	NumJoinsPerPlayerMap m_numJoinsPerPlayer;
	
	PlayerIdList m_autoLeavePlayerList;
	mutable Mutex m_autoLeavePlayerListMutex;
	
	PlayerIdList m_playerInvitationList;
	mutable Mutex m_playerInvitationListMutex;

	PlayerDataList m_computerPlayerList;
	mutable Mutex m_computerPlayerListMutex;

	PlayerIdList m_reportedAvatarList;
};

END_UPP_NAMESPACE

#endif