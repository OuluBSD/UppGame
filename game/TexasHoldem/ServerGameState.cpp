#include "ServerGameState.h"
#include "ServerGame.h"
#include "NetPacket.h"
#include "ServerLobbyThread.h"
#include "SessionManager.h"
#include <GameRules/HandInterface.h>
#include <GameRules/BeroInterface.h>
#include <GameRules/Game.h>

NAMESPACE_UPP

// AbstractServerGameStateReceiving
void AbstractServerGameStateReceiving::ProcessPacket(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet)
{
	InternalProcessPacket(server, session, packet);
}

std::shared_ptr<NetPacket> AbstractServerGameStateReceiving::CreateNetPacketPlayerJoined(unsigned gameId, const PlayerData &playerData) { 
	auto pkt = std::make_shared<NetPacket>();
	auto& msg = pkt->Create<GamePlayerJoinedMsg>();
	msg.gameId = gameId;
	msg.playerId = playerData.GetUniqueId();
	msg.playerName = playerData.GetName();
	return pkt;
}

std::shared_ptr<NetPacket> AbstractServerGameStateReceiving::CreateNetPacketSpectatorJoined(unsigned gameId, const PlayerData &playerData) { return std::make_shared<NetPacket>(); }

std::shared_ptr<NetPacket> AbstractServerGameStateReceiving::CreateNetPacketJoinGameAck(const ServerGame &server, const PlayerData &playerData, bool spectateOnly) {
	auto pkt = std::make_shared<NetPacket>();
	auto& msg = pkt->Create<JoinGameAckMsg>();
	msg.gameId = server.GetId();
	NetPacket::SetGameData(server.GetGameData(), msg.gameInfo);
	msg.isAdmin = (playerData.GetRights() == (int)netPlayerRightsAdmin);
	return pkt;
}

std::shared_ptr<NetPacket> AbstractServerGameStateReceiving::CreateNetPacketHandStart(const ServerGame &server) {
	auto pkt = std::make_shared<NetPacket>();
	auto& msg = pkt->Create<HandStartMsg>();
	msg.gameId = server.GetId();
	msg.smallBlind = server.GetGameData().firstSmallBlind;
	return pkt;
}

void AbstractServerGameStateReceiving::AcceptNewSession(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session, bool spectateOnly) {
	server->GetSessionManager().SendToSession(server->GetLobbyThread().GetSender(), session->GetId(), CreateNetPacketJoinGameAck(*server, *session->GetPlayerData(), spectateOnly));
	
	server->SendToAllButOnePlayers(CreateNetPacketPlayerJoined(server->GetId(), *session->GetPlayerData()), session->GetId(), 0);

	if (server->GetSessionManager().GetPlayerCount() >= (unsigned)server->GetGameData().maxNumberOfPlayers) {
		PlayerDataList pdl;
		auto sessions = server->GetSessionManager().GetAllSessions();
		for (auto& s : sessions) pdl.push_back(s->GetPlayerData());
		server->StartGame(pdl);
	}
}

// ServerGameStateInit
ServerGameStateInit& ServerGameStateInit::Instance()
{
	static ServerGameStateInit s_state;
	return s_state;
}

void ServerGameStateInit::Enter(std::shared_ptr<ServerGame> server) {}
void ServerGameStateInit::Exit(std::shared_ptr<ServerGame> server) {}
void ServerGameStateInit::NotifyGameAdminChanged(std::shared_ptr<ServerGame> server) {}
void ServerGameStateInit::NotifySessionRemoved(std::shared_ptr<ServerGame> server) {}
void ServerGameStateInit::HandleNewPlayer(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session) {
	AcceptNewSession(server, session, false);
}
void ServerGameStateInit::HandleNewSpectator(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session) {}
void ServerGameStateInit::InternalProcessPacket(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet) {}

// ServerGameStateHand
ServerGameStateHand& ServerGameStateHand::Instance()
{
	static ServerGameStateHand s_state;
	return s_state;
}

void ServerGameStateHand::Enter(std::shared_ptr<ServerGame> server) {
	auto pkt = std::make_shared<NetPacket>();
	auto& msg = pkt->Create<HandStartMsg>();
	msg.gameId = server->GetId();
	if (server->GetGame().getCurrentHand())
		msg.dealerPlayerId = server->GetGame().getCurrentHand()->getDealerPosition();
	server->SendToAllPlayers(pkt, 0);
}

void ServerGameStateHand::Exit(std::shared_ptr<ServerGame> server) {}
void ServerGameStateHand::NotifyGameAdminChanged(std::shared_ptr<ServerGame> server) {}
void ServerGameStateHand::NotifySessionRemoved(std::shared_ptr<ServerGame> server) {}
void ServerGameStateHand::HandleNewPlayer(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session) {}
void ServerGameStateHand::HandleNewSpectator(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session) {}

void ServerGameStateHand::ProcessPacket(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet) {
	int type = packet->GetType();
	if (type == NetPacket::Type_PlayersActionDoneMessage) {
		server->SendToAllPlayers(packet, 0);
	}
}

// ServerGameStateFinal
ServerGameStateFinal& ServerGameStateFinal::Instance()
{
	static ServerGameStateFinal s_state;
	return s_state;
}

void ServerGameStateFinal::Enter(std::shared_ptr<ServerGame> server) {}
void ServerGameStateFinal::Exit(std::shared_ptr<ServerGame> server) {}
void ServerGameStateFinal::NotifyGameAdminChanged(std::shared_ptr<ServerGame> server) {}
void ServerGameStateFinal::NotifySessionRemoved(std::shared_ptr<ServerGame> server) {}
void ServerGameStateFinal::HandleNewPlayer(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session) {}
void ServerGameStateFinal::HandleNewSpectator(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session) {}
void ServerGameStateFinal::ProcessPacket(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet) {}

END_UPP_NAMESPACE