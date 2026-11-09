#ifndef _CardEngine_ClientThread_h_
#define _CardEngine_ClientThread_h_

#include <Core/Core.h>
#include <vector>
#include "Thread.h"
#include "SessionData.h"
#include <GameRules/GuiInterface.h>
#include "ServerData.h"
#include <GameRules/PlayerData.h>
#include <GameRules/GameData.h>
#include "NetPacket.h"

NAMESPACE_UPP

class ClientContext;
class ClientState;
class SenderHelper;
class Game;
class AvatarManager;
class EngineLog;

#define SIZE_PING_BACKLOG 20

class PingData {
public:
	PingData() { Reset(); }
	void Reset() { pingValues.Clear(); }
	unsigned MinPing() {
		if (pingValues.IsEmpty()) return 0;
		int m = pingValues[0];
		for (int v : pingValues) if (v < m) m = v;
		return (unsigned)m;
	}
	unsigned MaxPing() {
		if (pingValues.IsEmpty()) return 0;
		int m = pingValues[0];
		for (int v : pingValues) if (v > m) m = v;
		return (unsigned)m;
	}
	unsigned AveragePing() {
		if (pingValues.IsEmpty()) return 0;
		int sum = 0;
		for (int v : pingValues) sum += v;
		return (unsigned)(sum / pingValues.GetCount());
	}
	void StartPing() { pingStop.Reset(); }
	bool EndPing() {
		pingValues.Add((int)pingStop.Elapsed());
		if (pingValues.GetCount() > SIZE_PING_BACKLOG) pingValues.Remove(0);
		return true;
	}
private:
	Vector<int> pingValues;
	TimeStop pingStop;
};

class ClientThread;
extern std::shared_ptr<ClientThread> g_clientThread;

class ClientThread : public ThreadWrapper, public std::enable_shared_from_this<ClientThread>, public SessionDataCallback {
public:
	ClientThread(GuiInterface &gui, AvatarManager &avatarManager, EngineLog *myLog);
	virtual ~ClientThread();

	void Init(const String &serverAddress, const String &serverListUrl, const String &serverPassword,
	          bool useServerList, unsigned serverPort, bool ipv6, bool sctp, bool tls,
	          const String &avatarServerAddress, const String &playerName,
	          const String &avatarFile, const String &cacheDir);

	virtual void SignalTermination() override;

	// Network send methods (Stubs)
	void SendKickPlayer(unsigned playerId) {}
	void SendLeaveCurrentGame() {}
	void SendStartEvent(bool fillUpWithCpuPlayers) {}
	void SendPlayerAction() {}
	void SendGameChatMessage(const String &msg) {}
	void SendLobbyChatMessage(const String &msg) {}
	void SendPrivateChatMessage(unsigned targetPlayerId, const String &msg) {}
	void SendJoinFirstGame(const String &password, bool autoLeave) {}
	void SendJoinGame(unsigned gameId, const String &password, bool autoLeave) {}
	void SendRejoinGame(unsigned gameId, bool autoLeave) {}
	void SendCreateGame(const GameData &gameData, const String &name, const String &password, bool autoLeave) {}
	void SendResetTimeout() {}
	void SendAskKickPlayer(unsigned playerId) {}
	void SendVoteKick(bool doKick) {}
	void SendShowMyCards() {}
	void SendInvitePlayerToCurrentGame(unsigned playerId) {}
	void SendRejectGameInvitation(unsigned gameId, DenyGameInvitationReason reason) {}
	void SendReportAvatar(unsigned reportedPlayerId, const String &avatarHash) {}
	void SendReportGameName(unsigned reportedGameId) {}
	void SendAdminRemoveGame(unsigned removeGameId) {}
	void SendAdminBanPlayer(unsigned playerId) {}

	void StartAsyncRead();
	void EnqueuePacket(std::shared_ptr<NetPacket> packet);
	virtual void SessionError(std::shared_ptr<SessionData> session, int errorCode) override;
	virtual void SessionTimeoutWarning(std::shared_ptr<SessionData> session, unsigned remainingSec) override {}
	void HandlePacket(std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet);

	void SelectServer(unsigned serverId);
	void SetLogin(const String &userName, const String &password, bool isGuest);
	ServerInfo GetServerInfo(unsigned serverId) const;

	GameInfo GetGameInfo(unsigned gameId) const;
	PlayerInfo GetPlayerInfo(unsigned playerId) const;
	bool GetPlayerIdFromName(const String &playerName, unsigned &playerId) const;
	unsigned GetGameIdOfPlayer(unsigned playerId) const;
	ServerStats GetStatData() const;
	unsigned GetGameId() const;
	unsigned GetGuiPlayerId() const;
	int GetOrigGuiPlayerNum() const;

	void SetGuiPlayerId(unsigned guiPlayerId);
	void SetGameId(unsigned id);

	GuiInterface &GetCallback() { return m_gui; }
	GuiInterface &GetGui() { return m_gui; }
	AvatarManager &GetAvatarManager() { return m_avatarManager; }

	Event<const Vector<GameListNewMsg::GameEntry>&> WhenGameListNew;
	Event<const Vector<PlayerListMsg::PlayerEntry>&> WhenPlayerList;
	Event<const String&, const String&> WhenChatMessage;
	Event<const HandStartMsg&> WhenHandStart;
	Event<const PlayersTurnMsg&> WhenPlayersTurn;
	Event<const PlayersActionDoneMsg&> WhenPlayersActionDone;
	Event<const JoinGameAckMsg&> WhenJoinGameAck;
	Event<const GamePlayerJoinedMsg&> WhenGamePlayerJoined;
	Event<const GamePlayerLeftMsg&> WhenGamePlayerLeft;
	Event<const GameStartInitialMsg&> WhenGameStartInitial;
	Event<const EndOfHandShowCardsMsg&> WhenEndOfHandShowCards;
	Event<const EndOfHandHideCardsMsg&> WhenEndOfHandHideCards;

protected:
	virtual void Main() override;

	void InitGame();
	void SendSessionPacket(std::shared_ptr<NetPacket> packet);
	void SendQueuedPackets();

	std::shared_ptr<Game> GetGame() { return m_game; }

private:
	struct LoginData {
		String userName;
		String password;
		bool isGuest = false;
	};

	std::shared_ptr<ClientContext> m_context;
	ClientState *m_curState;
	GuiInterface &m_gui;
	AvatarManager &m_avatarManager;
	EngineLog *m_clientLog;

	std::shared_ptr<SenderHelper> m_senderHelper;

	GameData m_gameData;
	StartData m_startData;
	PlayerDataList m_playerDataList;

	VectorMap<unsigned, ServerInfo> m_serverInfoMap;
	mutable Mutex m_serverInfoMapMutex;

	bool m_isServerSelected = false;
	unsigned m_selectedServerId = 0;
	mutable Mutex m_selectServerMutex;

	LoginData m_loginData;
	mutable Mutex m_loginDataMutex;

	VectorMap<unsigned, GameInfo> m_gameInfoMap;
	mutable Mutex m_gameInfoMapMutex;

	std::shared_ptr<Game> m_game;

	VectorMap<unsigned, PlayerInfo> m_playerInfoMap;
	mutable Mutex m_playerInfoMapMutex;

	unsigned m_curGameId = 0;
	mutable Mutex m_curGameIdMutex;

	unsigned m_curGameNum = 1;
	unsigned m_guiPlayerId = 0;
	mutable Mutex m_guiPlayerIdMutex;
	int m_origGuiPlayerNum = 0;
	bool m_sessionEstablished = false;

	ServerStats m_curStats;
	mutable Mutex m_curStatsMutex;

	PingData m_pingData;
	mutable Mutex m_pingDataMutex;

	std::vector<std::shared_ptr<NetPacket>> m_outPacketList;
};

END_UPP_NAMESPACE

#endif