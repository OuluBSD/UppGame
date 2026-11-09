#ifndef _CardEngine_ServerAcceptHelper_h_
#define _CardEngine_ServerAcceptHelper_h_

#include "ServerAcceptInterface.h"
#include "Thread.h"
#include "ServerLobbyThread.h"
#include "SessionData.h"

NAMESPACE_UPP

class ServerAcceptHelper : public ServerAcceptInterface, public ThreadWrapper
{
public:
	ServerAcceptHelper(GuiInterface &gui, bool tls)
		: m_gui(gui), m_tls(tls) {}

	virtual void Listen(unsigned serverPort, bool ipv6, const String &logDir,
						std::shared_ptr<ServerLobbyThread> lobbyThread) override
	{
		m_lobbyThread = lobbyThread;
		if (!m_server.Listen(serverPort, 128)) {
			// Handle error
			return;
		}
		Run();
	}

	virtual void Close() override
	{
		m_server.Close();
		SignalTermination();
	}

protected:
	virtual void Main() override
	{
		while (!ShouldTerminate()) {
			TcpSocket client;
			if (m_server.Accept(client)) {
				auto session = std::make_shared<SessionData>(m_lobbyThread->GetNextSessionId(), m_lobbyThread->GetSessionDataCallback());
				session->GetSocket() = std::move(client);
				m_lobbyThread->AddConnection(session);
			} else {
				Msleep(10);
			}
		}
	}

private:
	GuiInterface &m_gui;
	TcpSocket m_server;
	bool m_tls;
	std::shared_ptr<ServerLobbyThread> m_lobbyThread;
};

END_UPP_NAMESPACE

#endif
