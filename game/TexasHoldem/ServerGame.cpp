#include "ServerGame.h"
#include "ServerLobbyThread.h"
#include "NetPacket.h"
#include <GameRules/Game.h>
#include <GameRules/PlayerInterface.h>
#include <GameRules/HandInterface.h>
#include <GameRules/EngineLog.h>
#include "SenderHelper.h"
#include <GameRules/EngineDefs.h>
#include <CtrlLib/CtrlLib.h>

NAMESPACE_UPP

ServerGame::ServerGame(std::shared_ptr<ServerLobbyThread> lobbyThread, uint32 id, const String &name, const String &pwd, const GameData &gameData,
					   unsigned adminPlayerId, unsigned creatorPlayerDBId, GuiInterface &gui, class ConfigFile &playerConfig)
	: m_adminPlayerId(adminPlayerId), m_lobbyThread(lobbyThread), m_gui(gui),
	  m_gameData(gameData), m_curState(nullptr), m_id(id), m_name(name),
	  m_password(pwd), m_creatorPlayerDBId(creatorPlayerDBId), m_playerConfig(playerConfig),
	  m_gameNum(1), m_curPetitionId(1), m_isNameReported(false)
{
}

ServerGame::~ServerGame() {}

void ServerGame::Init()
{
	SetState(ServerGameStateInit::Instance());
}

void ServerGame::Exit()
{
	SetState(ServerGameStateFinal::Instance());
}

uint32 ServerGame::GetId() const { return m_id; }
const String& ServerGame::GetName() const { return m_name; }
unsigned ServerGame::GetCreatorDBId() const { return m_creatorPlayerDBId; }

void ServerGame::AddSession(std::shared_ptr<SessionData> session, bool spectateOnly)
{
	if (session) {
		if (spectateOnly) GetState().HandleNewSpectator(shared_from_this(), session);
		else GetState().HandleNewPlayer(shared_from_this(), session);
	}
}

void ServerGame::RemovePlayer(unsigned playerId, unsigned errorCode)
{
	std::shared_ptr<SessionData> tmpSession = m_sessionManager.GetSessionByUniquePlayerId(playerId);
	if (tmpSession) {
		SessionError(tmpSession, errorCode);
	}
}

void ServerGame::MutePlayer(unsigned playerId, bool mute)
{
	if (m_game) {
		auto tmpPlayer = m_game->getPlayerByUniqueId(playerId);
		if (tmpPlayer) tmpPlayer->setIsMuted(mute);
	}
}

void ServerGame::MarkPlayerAsInactive(unsigned playerId)
{
	if (m_game) {
		auto tmpPlayer = m_game->getPlayerByUniqueId(playerId);
		if (tmpPlayer) tmpPlayer->setIsSessionActive(false);
	}
}

void ServerGame::MarkPlayerAsKicked(unsigned playerId)
{
	if (m_game) {
		auto tmpPlayer = m_game->getPlayerByUniqueId(playerId);
		if (tmpPlayer) {
			tmpPlayer->setIsKicked(true);
			tmpPlayer->setMyGuid("");
		}
	}
}

void ServerGame::HandlePacket(std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet)
{
	if (session && packet) GetState().ProcessPacket(shared_from_this(), session, packet);
}

TexasRound ServerGame::GetCurRound() const
{
	if (m_game && m_game->getCurrentHand())
		return (TexasRound)m_game->getCurrentHand()->getCurrentRound();
	return GAME_STATE_PREFLOP;
}

void ServerGame::SendToAllPlayers(std::shared_ptr<NetPacket> packet, int state)
{
	m_sessionManager.SendToAllSessions(m_lobbyThread->GetSender(), packet, state);
}

void ServerGame::SendToAllButOnePlayers(std::shared_ptr<NetPacket> packet, SessionId except, int state)
{
	m_sessionManager.SendToAllButOneSessions(m_lobbyThread->GetSender(), packet, except, state);
}

void ServerGame::RemoveAllSessions()
{
	SetState(ServerGameStateFinal::Instance());
	auto sessions = m_sessionManager.GetAllSessions();
	for (auto& s : sessions) {
		if (s) {
			s->SetState(SessionData::Closed);
			s->CancelTimers();
			s->CloseSocketHandle();
		}
	}
	m_sessionManager.Clear();
}

void ServerGame::MoveSpectatorsToLobby()
{
	PlayerIdList spectatorList = GetSpectatorIdList();
	for (unsigned id : spectatorList) {
		std::shared_ptr<SessionData> tmpSession = m_sessionManager.GetSessionByUniquePlayerId(id);
		if (tmpSession)
			MoveSessionToLobby(tmpSession, NTF_NET_REMOVED_GAME_CLOSED);
	}
}

bool ServerGame::IsPasswordProtected() const { return !m_password.IsEmpty(); }
bool ServerGame::CheckPassword(const String &password) const { return password == m_password; }

bool ServerGame::CheckSettings(const GameData &data, const String &password, ServerMode mode)
{
	if (mode != SERVER_MODE_LAN && data.playerActionTimeoutSec < 5) return false;
	if (data.gameType == NET_GAME_TYPE_RANKING) {
		if (data.startMoney != RANKING_GAME_START_CASH || data.maxNumberOfPlayers != RANKING_GAME_NUMBER_OF_PLAYERS ||
		    data.firstSmallBlind != RANKING_GAME_START_SBLIND || !password.IsEmpty() || !data.allowSpectators)
			return false;
	}
	return true;
}

std::shared_ptr<PlayerData> ServerGame::GetPlayerDataByUniqueId(unsigned playerId) const
{
	auto session = m_sessionManager.GetSessionByUniquePlayerId(playerId);
	if (session) return session->GetPlayerData();
	return nullptr;
}

PlayerIdList ServerGame::GetPlayerIdList() const { return m_sessionManager.GetPlayerIdList(SessionData::Game); }
PlayerIdList ServerGame::GetSpectatorIdList() const { return m_sessionManager.GetPlayerIdList(SessionData::Spectating); }

bool ServerGame::IsPlayerConnected(const String &name) const { return m_sessionManager.IsPlayerConnected(name); }
bool ServerGame::IsPlayerConnected(unsigned playerId) const { return m_sessionManager.IsPlayerConnected(playerId); }
bool ServerGame::IsClientAddressConnected(const String &clientAddress) const { return m_sessionManager.IsClientAddressConnected(clientAddress); }

std::shared_ptr<PlayerInterface> ServerGame::GetPlayerInterfaceFromGame(const String &playerName)
{
	return m_game ? m_game->getPlayerByName(playerName) : nullptr;
}

std::shared_ptr<PlayerInterface> ServerGame::GetPlayerInterfaceFromGame(unsigned playerId)
{
	return m_game ? m_game->getPlayerByUniqueId(playerId) : nullptr;
}

bool ServerGame::IsRunning() const { return m_game != nullptr; }

void ServerGame::StartGame(const PlayerDataList &playerDataList)
{
	m_game = std::make_shared<Game>(&m_gui, m_lobbyThread->GetEngineFactory(), playerDataList, m_gameData, m_startData, m_id, &m_lobbyThread->GetLog(), &m_playerConfig);
}

unsigned ServerGame::GetAdminPlayerId() const { return m_adminPlayerId; }
void ServerGame::SetAdminPlayerId(unsigned playerId) { m_adminPlayerId = playerId; }

void ServerGame::AddPlayerInvitation(unsigned playerId)
{
	Mutex::Lock lock(m_playerInvitationListMutex);
	bool found = false;
	for (unsigned id : m_playerInvitationList) if (id == playerId) { found = true; break; }
	if (!found) m_playerInvitationList.Add(playerId);
}

void ServerGame::RemovePlayerInvitation(unsigned playerId)
{
	Mutex::Lock lock(m_playerInvitationListMutex);
	for (int i = 0; i < m_playerInvitationList.GetCount(); i++) {
		if (m_playerInvitationList[i] == playerId) {
			m_playerInvitationList.Remove(i);
			break;
		}
	}
}

bool ServerGame::IsPlayerInvited(unsigned playerId) const
{
	Mutex::Lock lock(m_playerInvitationListMutex);
	for (unsigned id : m_playerInvitationList) if (id == playerId) return true;
	return false;
}

void ServerGame::SetPlayerAutoLeaveOnFinish(unsigned playerId)
{
	Mutex::Lock lock(m_autoLeavePlayerListMutex);
	m_autoLeavePlayerList.Add(playerId);
}

void ServerGame::SetNameReported() { m_isNameReported = true; }
bool ServerGame::IsNameReported() const { return m_isNameReported; }

SessionManager& ServerGame::GetSessionManager() { return m_sessionManager; }
const SessionManager& ServerGame::GetSessionManager() const { return m_sessionManager; }
ServerLobbyThread& ServerGame::GetLobbyThread() { return *m_lobbyThread; }
GuiInterface& ServerGame::GetGui() { return m_gui; }
ServerGameState& ServerGame::GetState() { return *m_curState; }

void ServerGame::SetState(ServerGameState &newState)
{
	if (m_curState) m_curState->Exit(shared_from_this());
	m_curState = &newState;
	m_curState->Enter(shared_from_this());
}

Game& ServerGame::GetGame() { return *m_game; }
const Game& ServerGame::GetGame() const { return *m_game; }
const GameData& ServerGame::GetGameData() const { return m_gameData; }
const StartData& ServerGame::GetStartData() const { return m_startData; }

void ServerGame::SessionError(std::shared_ptr<SessionData> session, int errorCode)
{
	RemoveSession(session, NTF_NET_INTERNAL);
	m_lobbyThread->SessionError(session, errorCode);
}

void ServerGame::RemoveSession(std::shared_ptr<SessionData> session, int reason)
{
	if (m_sessionManager.RemoveSession(session->GetId())) {
		// player removal logic...
	}
}

void ServerGame::MoveSessionToLobby(std::shared_ptr<SessionData> session, int reason)
{
	RemoveSession(session, reason);
	session->ResetReadyFlag();
	m_lobbyThread->ReAddSession(session, reason, GetId());
}

END_UPP_NAMESPACE
