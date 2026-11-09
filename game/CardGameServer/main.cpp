#include "CardGameServer.h"

CardGameServer::CardGameServer()
{
}

bool CardGameServer::Start(int port)
{
	if(!listener_.Listen(port, 5)) {
		Cout() << "CardGameServer: Failed to listen on port " << port << "\n";
		return false;
	}
	running_ = true;
	Cout() << "CardGameServer: Listening for strategy requests on port " << port << "...\n";
	return true;
}

void CardGameServer::Stop()
{
	running_ = false;
	listener_.Close();
}

CardGameSuggestResponse CardGameServer::EvaluateRequest(const CardGameSuggestRequest& req)
{
	int64 start_ts = msecs();
	CardGameSuggestResponse resp;
	resp.protocol_version = req.protocol_version;
	resp.request_id       = req.request_id;
	resp.table_id         = req.table_id;

	CardGamePathInfo info;
	if(!info.Parse(req.compact_path)) {
		resp.status = "invalid_path";
		resp.latency_ms = (int)(msecs() - start_ts);
		return resp;
	}

	if(info.in_turn != req.hero_seat && req.hero_seat >= 0) {
		resp.status = "not_hero_turn";
		resp.latency_ms = (int)(msecs() - start_ts);
		return resp;
	}

	resp.status = "ok";
	resp.unit   = info.unit;

	// Heuristic strategy evaluator
	if(info.hole_cards.GetCount() >= 2) {
		String c1 = info.hole_cards[0];
		String c2 = info.hole_cards[1];
		bool is_pair = (c1.GetCount() > 0 && c2.GetCount() > 0 && c1[0] == c2[0]);

		if(is_pair || c1[0] == 'A' || c2[0] == 'A' || c1[0] == 'K' || c2[0] == 'K') {
			resp.primary_action = "raise";
			resp.amount = info.big_blind > 0 ? (info.big_blind * 3) : 6.0;
			resp.freq_fold = 0.05;
			resp.freq_call = 0.25;
			resp.freq_raise = 0.70;
			resp.equity = 0.72;
			resp.formatted = (resp.unit == "$") ? Format("Raise to $%.2f", resp.amount) : Format("Raise to %.2f %s", resp.amount, resp.unit);
		}
		else {
			resp.primary_action = "call";
			resp.amount = info.big_blind > 0 ? info.big_blind : 2.0;
			resp.freq_fold = 0.20;
			resp.freq_call = 0.65;
			resp.freq_raise = 0.15;
			resp.equity = 0.52;
			resp.formatted = (resp.unit == "$") ? Format("Call $%.2f", resp.amount) : Format("Call %.2f %s", resp.amount, resp.unit);
		}
	}
	else {
		resp.primary_action = "check";
		resp.amount = 0.0;
		resp.freq_fold = 0.10;
		resp.freq_call = 0.80;
		resp.freq_raise = 0.10;
		resp.equity = 0.50;
		resp.formatted = "Check";
	}

	resp.latency_ms = (int)(msecs() - start_ts);
	return resp;
}

void CardGameServer::ProcessClient(TcpSocket& client)
{
	String json_in;
	while(running_ && client.IsOpen()) {
		if(ReadJsonMessage(client, json_in, 1000)) {
			CardGameSuggestRequest req;
			if(req.FromJson(json_in)) {
				Cout() << "CardGameServer: Received request " << req.request_id << " for table " << req.table_id << "\n";
				CardGameSuggestResponse resp = EvaluateRequest(req);
				String json_out = resp.ToJson();
				SendJsonMessage(client, json_out);
				Cout() << "CardGameServer: Responded: " << resp.formatted << " (latency " << resp.latency_ms << "ms)\n";
			}
		}
	}
}

void CardGameServer::Run()
{
	while(running_) {
		TcpSocket client;
		if(client.Accept(listener_)) {
			Cout() << "CardGameServer: Client connected from " << client.GetPeerAddr() << "\n";
			ProcessClient(client);
			Cout() << "CardGameServer: Client disconnected.\n";
		}
		Sleep(10);
	}
}

CONSOLE_APP_MAIN
{
	int port = 8765;
	bool selftest = false;

	const Vector<String>& cmd = CommandLine();
	for(int i = 0; i < cmd.GetCount(); i++) {
		if(cmd[i] == "--selftest") selftest = true;
		if(cmd[i].StartsWith("--port=")) port = StrInt(cmd[i].Mid(7));
	}

	CardGameServer server;
	if(selftest) {
		Cout() << "CardGameServer: Running selftest...\n";
		CardGameSuggestRequest req;
		req.request_id = "test_1";
		req.table_id = 0;
		req.compact_path = "p2/u($)/d0/s0(100)/s1(100)/sb($0.50)/bb($1)/H(s1:Hero:AhAd)/s0:r($4)";
		req.hero_seat = 1;

		CardGameSuggestResponse resp = server.EvaluateRequest(req);
		ASSERT(resp.status == "ok");
		ASSERT(resp.primary_action == "raise");
		Cout() << "CardGameServer selftest PASSED: suggestion = " << resp.formatted << "\n";
		return;
	}

	if(server.Start(port)) {
		server.Run();
	}
}
