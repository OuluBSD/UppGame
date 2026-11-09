#ifndef _CardEngine_ServerAcceptInterface_h_
#define _CardEngine_ServerAcceptInterface_h_

#include <Core/Core.h>

NAMESPACE_UPP

class ServerLobbyThread;

class ServerAcceptInterface
{
public:
	virtual ~ServerAcceptInterface() {}

	virtual void Listen(unsigned serverPort, bool ipv6, const String &logDir,
						std::shared_ptr<ServerLobbyThread> lobbyThread) = 0;

	virtual void Close() = 0;
};

END_UPP_NAMESPACE

#endif
