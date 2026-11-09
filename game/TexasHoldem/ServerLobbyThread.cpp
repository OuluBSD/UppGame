#include "ServerLobbyThread.h"
#include "ServerGame.h"
#include "NetPacket.h"
#include <CtrlLib/CtrlLib.h>

NAMESPACE_UPP

ServerLobbyThread::ServerLobbyThread(GuiInterface &gui, ServerMode mode, class ConfigFile &serverConfig, AvatarManager &avatarManager)
	: m_gui(gui), m_mode(mode), m_serverConfig(serverConfig), m_avatarManager(avatarManager)
{
	m_sender = std::make_shared<SenderHelper>();
}

ServerLobbyThread::~ServerLobbyThread()
{
}

void ServerLobbyThread::Init(const String &logDir)
{
}

void ServerLobbyThread::SignalTermination()
{
	ThreadWrapper::SignalTermination();
}

void ServerLobbyThread::AddConnection(std::shared_ptr<SessionData> sessionData)
{
	m_sessionManager.AddSession(sessionData);
}

void ServerLobbyThread::ReAddSession(std::shared_ptr<SessionData> session, int reason, unsigned gameId)
{
	m_gameSessionManager.RemoveSession(session->GetId());
	m_sessionManager.AddSession(session);
}

void ServerLobbyThread::MoveSessionToGame(std::shared_ptr<ServerGame> game, std::shared_ptr<SessionData> session, bool autoLeave, bool spectateOnly)
{
	m_sessionManager.RemoveSession(session->GetId());
	m_gameSessionManager.AddSession(session);
	game->AddSession(session, spectateOnly);
}

void ServerLobbyThread::SessionError(std::shared_ptr<SessionData> session, int errorCode)
{
	CloseSession(session);
}

void ServerLobbyThread::DispatchPacket(std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet)
{
	HandlePacket(session, packet);
}

void ServerLobbyThread::CloseSession(std::shared_ptr<SessionData> session)
{
	m_sessionManager.RemoveSession(session->GetId());
	m_gameSessionManager.RemoveSession(session->GetId());
	session->CloseSocketHandle();
}

void ServerLobbyThread::UpdateStatisticsNumberOfPlayers()
{
}

uint32 ServerLobbyThread::GetNextSessionId()
{
	return m_curSessionId++;
}

uint32 ServerLobbyThread::GetNextUniquePlayerId()
{
	return ++m_curUniquePlayerId;
}

uint32 ServerLobbyThread::GetNextGameId()
{
	return ++m_curGameId;
}

ServerStats ServerLobbyThread::GetStats() const
{
	Mutex::Lock lock(m_statMutex);
	return m_statData;
}

ServerMode ServerLobbyThread::GetServerMode() const
{
	return m_mode;
}

void ServerLobbyThread::Main()
{
	VectorMap<SessionId, String> receiveBuffers;

	while (!ShouldTerminate()) {
		auto sessions = m_sessionManager.GetAllSessions();
		for (auto& s : sessions) {
			if (s->GetSocket().IsOpen() && s->GetSocket().Wait(WAIT_READ, 0)) {
				char buffer[4096];
				int n = s->GetSocket().Recv(buffer, sizeof(buffer));
				if (n > 0) {
					String& rb = receiveBuffers.GetAdd(s->GetId());
					rb.Cat(buffer, n);
					while (rb.GetCount() >= 4) {
						uint32 size = Peek32le(~rb);
						if (rb.GetCount() >= (int)(4 + size)) {
							String pktData = rb.Mid(4, size);
							rb.Remove(0, 4 + size);
							
							auto pkt = std::make_shared<NetPacket>();
							StringStream ss(pktData);
							pkt->Serialize(ss);
							HandlePacket(s, pkt);
						} else break;
					}
				} else if (n == 0) {
					receiveBuffers.RemoveKey(s->GetId());
					CloseSession(s);
				}
			}
		}
		
		auto gameSessions = m_gameSessionManager.GetAllSessions();
		for (auto& s : gameSessions) {
			if (s->GetSocket().IsOpen() && s->GetSocket().Wait(WAIT_READ, 0)) {
				char buffer[4096];
				int n = s->GetSocket().Recv(buffer, sizeof(buffer));
				if (n > 0) {
					String& rb = receiveBuffers.GetAdd(s->GetId());
					rb.Cat(buffer, n);
					while (rb.GetCount() >= 4) {
						uint32 size = Peek32le(~rb);
						if (rb.GetCount() >= (int)(4 + size)) {
							String pktData = rb.Mid(4, size);
							rb.Remove(0, 4 + size);
							
							auto pkt = std::make_shared<NetPacket>();
							StringStream ss(pktData);
							pkt->Serialize(ss);
							// Find game and dispatch
						} else break;
					}
				} else if (n == 0) {
					receiveBuffers.RemoveKey(s->GetId());
					CloseSession(s);
				}
			}
		}
		Msleep(10);
	}
}

void ServerLobbyThread::RegisterTimers()
{
}

void ServerLobbyThread::CancelTimers()
{
}

void ServerLobbyThread::HandlePacket(std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet)
{
}

SessionDataCallback& ServerLobbyThread::GetSessionDataCallback()
{
	static struct DummyCallback : public SessionDataCallback {
		virtual void SessionError(std::shared_ptr<SessionData> session, int errorCode) override {}
		virtual void SessionTimeoutWarning(std::shared_ptr<SessionData> session, unsigned remainingSec) override {}
	} dummy;
	return dummy;
}

END_UPP_NAMESPACE