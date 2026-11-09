#ifndef _CardEngine_SessionData_h_
#define _CardEngine_SessionData_h_

#include <GameRules/EngineDefs.h>
#include <GameRules/GameDefs.h>

NAMESPACE_UPP

class ServerGame;
class PlayerData;
class ReceiveBuffer;
class SendBuffer;
class SessionData;

class SessionDataCallback {
public:
	virtual ~SessionDataCallback() {}
	virtual void SessionError(std::shared_ptr<SessionData> session, int error) = 0;
	virtual void SessionTimeoutWarning(std::shared_ptr<SessionData> session, unsigned remainingSec) = 0;
};

class SessionData : public std::enable_shared_from_this<SessionData> {
public:
	enum State {
		Init,
		Established,
		InGame,
		Game, // Alias for InGame if needed by original code
		Spectating,
		SpectatorWaiting,
		Disconnected,
		Closed
	};

	SessionData(SessionId id, SessionDataCallback &cb);
	~SessionData();

	SessionId GetId() const;

	std::shared_ptr<ServerGame> GetGame() const;
	void SetGame(std::shared_ptr<ServerGame> game);

	State GetState() const;
	void SetState(State state);

	PKRTcpSocket& GetSocket() { return m_socket; }

	void SetReadyFlag();
	void ResetReadyFlag();
	bool IsReady() const;

	void SetWantsLobbyMsg();
	void ResetWantsLobbyMsg();
	bool WantsLobbyMsg() const;

	const String& GetClientAddr() const;
	void SetClientAddr(const String &addr);

	void CloseSocketHandle();

	void StartTimerInitTimeout(unsigned timeoutSec);
	void StartTimerGlobalTimeout(unsigned timeoutSec);
	void ResetGlobalTimeout();
	void StartTimerActivityTimeout(unsigned timeoutSec, unsigned warningRemainingSec);
	void ResetActivityTimer();
	void CancelTimers();

	void SetPlayerData(std::shared_ptr<PlayerData> player);
	std::shared_ptr<PlayerData> GetPlayerData();

	String GetRemoteIPAddressFromSocket() const;

	std::shared_ptr<ReceiveBuffer> GetReceiveBuffer() { return m_receiveBuffer; }
	std::shared_ptr<SendBuffer> GetSendBuffer() { return m_sendBuffer; }

private:
	void InternalClearAuthSession();

	PKRTcpSocket m_socket;
	SessionId m_id;
	std::weak_ptr<ServerGame> m_game;
	State m_state;
	String m_clientAddr;
	std::shared_ptr<ReceiveBuffer> m_receiveBuffer;
	std::shared_ptr<SendBuffer> m_sendBuffer;
	std::shared_ptr<PlayerData> m_playerData;
	bool m_readyFlag;
	bool m_wantsLobbyMsg;

	unsigned m_activityTimeoutSec;
	unsigned m_activityWarningRemainingSec;
	unsigned m_globalTimeoutSec;

	SessionDataCallback &m_callback;
	mutable Mutex m_dataMutex;

	void* m_authSession; // Stub
	int m_curAuthStep;
	String m_password;
};

END_UPP_NAMESPACE

#endif