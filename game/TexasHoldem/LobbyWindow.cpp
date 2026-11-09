#include "LobbyWindow.h"
#include "CreateGameWindow.h"
#include "ClientThread.h"
#include "NetPacket.h"
#include "GameTable.h"

NAMESPACE_UPP

extern std::shared_ptr<ClientThread> g_clientThread;

LobbyWindow::LobbyWindow()
{
	CtrlLayout(*this);
	Title("PKR Lobby");
	
	gameList.AddColumn("ID");
	gameList.AddColumn("Game Name");
	gameList.AddColumn("Players");
	
	playerList.AddColumn("ID");
	playerList.AddColumn("Nick Name");
	
	btnSend << [this] { OnSend(); };
	btnCreate << [this] { OnCreate(); };
	btnJoin << [this] { OnJoin(); };
	chatInput.WhenAction = [this] { OnSend(); };
	
	if (g_clientThread) {
		g_clientThread->WhenGameListNew = [this](const Vector<GameListNewMsg::GameEntry>& games) {
			gameList.Clear();
			for (const auto& g : games) {
				AddGame(g.gameId, g.name, g.players, g.maxPlayers);
			}
		};
		g_clientThread->WhenPlayerList = [this](const Vector<PlayerListMsg::PlayerEntry>& players) {
			playerList.Clear();
			for (const auto& p : players) {
				AddPlayer(p.playerId, p.name);
			}
		};
		g_clientThread->WhenChatMessage = [this](const String& name, const String& text) {
			AddChatMessage(name, text);
		};
		g_clientThread->WhenJoinGameAck = [this](const JoinGameAckMsg& msg) {
			GameTable pt;
			pt.Run();
		};
	}
}

void LobbyWindow::AddGame(unsigned id, const String& name, int players, int maxPlayers)
{
	gameList.Add((int)id, name, Format("%d/%d", players, maxPlayers));
}

void LobbyWindow::AddPlayer(unsigned id, const String& name)
{
	playerList.Add((int)id, name);
}

void LobbyWindow::AddChatMessage(const String& name, const String& text)
{
	chatDisplay.SetQTF(chatDisplay.GetQTF() + Format("[* %s]: %s&", name, text));
}

void LobbyWindow::OnSend()
{
	String txt = chatInput.GetData();
	if (!txt.IsEmpty()) {
		if (g_clientThread) {
			auto pkt = std::make_shared<NetPacket>();
			auto& msg = pkt->Create<ChatMsg>();
			msg.chatText = txt;
			msg.chatType = 1; // Lobby chat
			g_clientThread->EnqueuePacket(pkt);
		}
		chatInput.Clear();
	}
}

void LobbyWindow::OnJoin()
{
	if (gameList.IsCursor()) {
		unsigned gameId = (int)gameList.Get(gameList.GetCursor(), 0);
		if (g_clientThread) {
			auto pkt = std::make_shared<NetPacket>();
			auto& msg = pkt->Create<JoinExistingGameMsg>();
			msg.gameId = gameId;
			msg.autoLeave = true;
			g_clientThread->EnqueuePacket(pkt);
		}
	}
}

void LobbyWindow::OnCreate()
{
	CreateGameWindow dlg;
	if (dlg.Run() == IDOK) {
		if (g_clientThread) {
			auto pkt = std::make_shared<NetPacket>();
			auto& msg = pkt->Create<JoinNewGameMsg>();
			NetPacket::SetGameData(dlg.GetGameData(), msg.gameInfo);
			msg.gameInfo.gameName = dlg.GetGameName();
			msg.autoLeave = true;
			g_clientThread->EnqueuePacket(pkt);
		}
	}
}

END_UPP_NAMESPACE
