#include "CardGameClient.h"

CardGameClient::CardGameClient()
{
}

bool CardGameClient::Connect(const String& host, int port)
{
	if(!socket_.Connect(host, port)) {
		Cout() << "CardGameClient: Could not connect to strategy server at " << host << ":" << port << "\n";
		return false;
	}
	Cout() << "CardGameClient: Connected to strategy server at " << host << ":" << port << "\n";
	return true;
}

void CardGameClient::Disconnect()
{
	socket_.Close();
}

bool CardGameClient::RequestSuggestion(const String& compact_path, int table_id, int hero_seat, CardGameSuggestResponse& out_resp, int timeout_ms)
{
	if(!socket_.IsOpen()) return false;

	req_counter_++;
	CardGameSuggestRequest req;
	req.request_id = Format("req_%d", req_counter_);
	req.table_id = table_id;
	req.compact_path = compact_path;
	req.hero_seat = hero_seat;
	req.hero_name = "Hero";
	req.time_limit_ms = timeout_ms;

	String json_req = req.ToJson();
	if(!SendJsonMessage(socket_, json_req)) {
		Cout() << "CardGameClient: Failed to send request message.\n";
		return false;
	}

	String json_resp;
	if(!ReadJsonMessage(socket_, json_resp, timeout_ms)) {
		Cout() << "CardGameClient: Read timeout waiting for server response.\n";
		return false;
	}

	return out_resp.FromJson(json_resp);
}

CONSOLE_APP_MAIN
{
	String host = "127.0.0.1";
	int port = 8765;
	bool selftest = false;
	String compact_path = "p2/u($)/d0/s0(100)/s1(100)/sb($0.50)/bb($1)/H(s1:Hero:AhAd)";

	const Vector<String>& cmd = CommandLine();
	for(int i = 0; i < cmd.GetCount(); i++) {
		if(cmd[i] == "--selftest") selftest = true;
		if(cmd[i].StartsWith("--host=")) host = cmd[i].Mid(7);
		if(cmd[i].StartsWith("--port=")) port = StrInt(cmd[i].Mid(7));
		if(cmd[i].StartsWith("--path=")) compact_path = cmd[i].Mid(7);
	}

	if(selftest) {
		Cout() << "CardGameClient: Running client selftest...\n";
		CardGamePathInfo info;
		ASSERT(info.Parse(compact_path));
		ASSERT(info.players == 2);
		ASSERT(info.hero_seat == 1);
		ASSERT(info.hole_cards.GetCount() == 2);
		Cout() << "CardGameClient selftest PASSED (Path parsed successfully: " << info.hole_cards[0] << " " << info.hole_cards[1] << ").\n";
		return;
	}

	CardGameClient client;
	if(client.Connect(host, port)) {
		CardGameSuggestResponse resp;
		if(client.RequestSuggestion(compact_path, 0, 1, resp)) {
			Cout() << "CardGameClient: Server Suggestion => " << resp.formatted
			       << " (status: " << resp.status << ", equity: " << resp.equity << ", latency: " << resp.latency_ms << "ms)\n";
		}
		client.Disconnect();
	}
}
