#ifndef _CardGameServer_CardGameServer_h_
#define _CardGameServer_CardGameServer_h_

#include <Core/Game/Game.h>

using namespace Upp;

class CardGameServer {
public:
	CardGameServer();

	bool Start(int port = 8765);
	void Run();
	void Stop();

	CardGameSuggestResponse EvaluateRequest(const CardGameSuggestRequest& req);

private:
	void ProcessClient(TcpSocket& client);

	TcpSocket listener_;
	bool      running_ = false;
};

#endif
