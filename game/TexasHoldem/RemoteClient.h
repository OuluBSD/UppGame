#ifndef _CardEngine_RemoteClient_h_
#define _CardEngine_RemoteClient_h_

#include <Core/Core.h>
#include "ScreenGameState.h"

namespace Upp {

class RemoteClient {
	TcpSocket socket;
	Thread    client_thread;
	bool      running = false;
	String    host;
	int       port;

public:
	RemoteClient();
	~RemoteClient();

	void Start(const String& addr); // addr as "host:port"
	void Stop();
	
	Event<const ScreenGameState&> WhenUpdate;

private:
	void Run();
};

}

#endif
