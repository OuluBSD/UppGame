#include "SessionData.h"
#include <CtrlLib/CtrlLib.h>
#include "ReceiveBuffer.h"
#include "SendBuffer.h"
#include "ServerGame.h"
#include <GameRules/PlayerData.h>
#include <GameRules/EngineLog.h>

NAMESPACE_UPP

SessionData::SessionData(SessionId id, SessionDataCallback &cb)
	: m_id(id), m_state(Init), m_readyFlag(false), m_wantsLobbyMsg(true),
	  m_activityTimeoutSec(0), m_activityWarningRemainingSec(0), m_globalTimeoutSec(0),
	  m_callback(cb), m_authSession(nullptr), m_curAuthStep(0)
{
	m_receiveBuffer.reset(new ReceiveBuffer);
	m_sendBuffer.reset(new SendBuffer);
}

SessionData::~SessionData()
{
	InternalClearAuthSession();
	CancelTimers();
	CloseSocketHandle();
}

SessionId SessionData::GetId() const
{
	return m_id;
}

std::shared_ptr<ServerGame> SessionData::GetGame() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_game.lock();
}

void SessionData::SetGame(std::shared_ptr<ServerGame> game)
{
	Mutex::Lock lock(m_dataMutex);
	m_game = game;
}

SessionData::State SessionData::GetState() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_state;
}

void SessionData::SetState(SessionData::State state)
{
	Mutex::Lock lock(m_dataMutex);
	m_state = state;
}

void SessionData::SetReadyFlag()
{
	Mutex::Lock lock(m_dataMutex);
	m_readyFlag = true;
}

void SessionData::ResetReadyFlag()
{
	Mutex::Lock lock(m_dataMutex);
	m_readyFlag = false;
}

bool SessionData::IsReady() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_readyFlag;
}

void SessionData::SetWantsLobbyMsg()
{
	Mutex::Lock lock(m_dataMutex);
	m_wantsLobbyMsg = true;
}

void SessionData::ResetWantsLobbyMsg()
{
	Mutex::Lock lock(m_dataMutex);
	m_wantsLobbyMsg = false;
}

bool SessionData::WantsLobbyMsg() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_wantsLobbyMsg;
}

const String & SessionData::GetClientAddr() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_clientAddr;
}

void SessionData::SetClientAddr(const String &addr)
{
	Mutex::Lock lock(m_dataMutex);
	m_clientAddr = addr;
}

void SessionData::CloseSocketHandle()
{
	if (m_socket.IsOpen()) {
		m_socket.Close();
	}
}

void SessionData::StartTimerInitTimeout(unsigned timeoutSec)
{
	Mutex::Lock lock(m_dataMutex);
	KillTimeCallback((void*)(uintptr_t)1);
	SetTimeCallback((int)timeoutSec * 1000, [this] {
		if (GetState() == Init) {
			CloseSocketHandle();
			m_callback.SessionError(shared_from_this(), ERR_NET_SESSION_TIMED_OUT);
		}
	}, (void*)(uintptr_t)1);
}

void SessionData::StartTimerGlobalTimeout(unsigned timeoutSec)
{
	Mutex::Lock lock(m_dataMutex);
	m_globalTimeoutSec = timeoutSec;
	KillTimeCallback((void*)(uintptr_t)2);
	SetTimeCallback((int)timeoutSec * 1000, [this] {
		m_callback.SessionError(shared_from_this(), ERR_NET_SESSION_TIMED_OUT);
	}, (void*)(uintptr_t)2);
}

void SessionData::ResetGlobalTimeout()
{
	Mutex::Lock lock(m_dataMutex);
	if (m_globalTimeoutSec > 0) {
		StartTimerGlobalTimeout(m_globalTimeoutSec);
	}
}

void SessionData::StartTimerActivityTimeout(unsigned timeoutSec, unsigned warningRemainingSec)
{
	Mutex::Lock lock(m_dataMutex);
	m_activityTimeoutSec = timeoutSec;
	m_activityWarningRemainingSec = warningRemainingSec;
	ResetActivityTimer();
}

void SessionData::ResetActivityTimer()
{
	Mutex::Lock lock(m_dataMutex);
	KillTimeCallback((void*)(uintptr_t)3);
	if (m_activityTimeoutSec > m_activityWarningRemainingSec) {
		SetTimeCallback((int)(m_activityTimeoutSec - m_activityWarningRemainingSec) * 1000, [this] {
			m_callback.SessionTimeoutWarning(shared_from_this(), m_activityWarningRemainingSec);
			KillTimeCallback((void*)(uintptr_t)3);
			SetTimeCallback((int)m_activityWarningRemainingSec * 1000, [this] {
				m_callback.SessionError(shared_from_this(), ERR_NET_SESSION_TIMED_OUT);
			}, (void*)(uintptr_t)3);
		}, (void*)(uintptr_t)3);
	}
}

void SessionData::CancelTimers()
{
	Mutex::Lock lock(m_dataMutex);
	KillTimeCallback((void*)(uintptr_t)1);
	KillTimeCallback((void*)(uintptr_t)2);
	KillTimeCallback((void*)(uintptr_t)3);
}

void SessionData::SetPlayerData(std::shared_ptr<PlayerData> player)
{
	Mutex::Lock lock(m_dataMutex);
	m_playerData = player;
}

std::shared_ptr<PlayerData> SessionData::GetPlayerData()
{
	Mutex::Lock lock(m_dataMutex);
	return m_playerData;
}

String SessionData::GetRemoteIPAddressFromSocket() const
{
	Mutex::Lock lock(m_dataMutex);
	if (m_socket.IsOpen()) {
		return m_socket.GetPeerAddr();
	}
	return String();
}

void SessionData::InternalClearAuthSession()
{
	m_curAuthStep = 0;
}

END_UPP_NAMESPACE
