#ifndef _CardGameClient_CardGameClient_h_
#define _CardGameClient_CardGameClient_h_

#include <Core/Game/Game.h>

using namespace Upp;

class CardGameClient {
public:
	CardGameClient();

	bool Connect(const String& host = "127.0.0.1", int port = 8765);
	void Disconnect();

	bool RequestSuggestion(const String& compact_path, int table_id, int hero_seat, CardGameSuggestResponse& out_resp, int timeout_ms = 2000);

private:
	TcpSocket socket_;
	int       req_counter_ = 0;
};

#endif
