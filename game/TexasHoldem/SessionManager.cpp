#include "SessionManager.h"
#include "SenderHelper.h"
#include "NetPacket.h"

NAMESPACE_UPP

SessionManager::SessionManager() {}
SessionManager::~SessionManager() {}

void SessionManager::AddSession(std::shared_ptr<SessionData> sessionData)
{
	Mutex::Lock lock(m_sessionMapMutex);
	m_sessionMap[sessionData->GetId()] = sessionData;
}

void SessionManager::SetSessionPlayerData(SessionId session, std::shared_ptr<PlayerData> playerData)
{
	Mutex::Lock lock(m_sessionMapMutex);
	auto it = m_sessionMap.find(session);
	if (it != m_sessionMap.end()) it->second->SetPlayerData(playerData);
}

bool SessionManager::RemoveSession(SessionId session)
{
	Mutex::Lock lock(m_sessionMapMutex);
	return m_sessionMap.erase(session) > 0;
}

std::shared_ptr<SessionData> SessionManager::GetSessionById(SessionId id) const
{
	Mutex::Lock lock(m_sessionMapMutex);
	auto it = m_sessionMap.find(id);
	return (it != m_sessionMap.end()) ? it->second : nullptr;
}

std::shared_ptr<SessionData> SessionManager::GetSessionByPlayerName(const String &playerName) const
{
	Mutex::Lock lock(m_sessionMapMutex);
	for (auto& pair : m_sessionMap) {
		if (pair.second->GetPlayerData() && pair.second->GetPlayerData()->GetName() == playerName)
			return pair.second;
	}
	return nullptr;
}

std::shared_ptr<SessionData> SessionManager::GetSessionByUniquePlayerId(unsigned uniqueId, bool initSessions) const
{
	Mutex::Lock lock(m_sessionMapMutex);
	for (auto& pair : m_sessionMap) {
		if (pair.second->GetPlayerData() && pair.second->GetPlayerData()->GetUniqueId() == uniqueId)
			return pair.second;
	}
	return nullptr;
}

std::vector<std::shared_ptr<SessionData>> SessionManager::GetAllSessions() const
{
	Mutex::Lock lock(m_sessionMapMutex);
	std::vector<std::shared_ptr<SessionData>> sessions;
	for (auto& pair : m_sessionMap) sessions.push_back(pair.second);
	return sessions;
}

PlayerDataList SessionManager::GetPlayerDataList() const
{
	Mutex::Lock lock(m_sessionMapMutex);
	PlayerDataList list;
	for (auto& pair : m_sessionMap) if (pair.second->GetPlayerData()) list.push_back(pair.second->GetPlayerData());
	return list;
}

PlayerDataList SessionManager::GetSpectatorDataList() const
{
	return PlayerDataList();
}

PlayerIdList SessionManager::GetPlayerIdList(int state) const
{
	return PlayerIdList();
}

bool SessionManager::IsPlayerConnected(const String &playerName) const
{
	return GetSessionByPlayerName(playerName) != nullptr;
}

bool SessionManager::IsPlayerConnected(unsigned uniqueId) const
{
	return GetSessionByUniquePlayerId(uniqueId) != nullptr;
}

bool SessionManager::IsClientAddressConnected(const String &clientAddress) const
{
	return false;
}

bool SessionManager::IsGuestAllowedToConnect(const String &clientAddress) const
{
	return true;
}

void SessionManager::ForEach(std::function<void (std::shared_ptr<SessionData>)> func)
{
	Mutex::Lock lock(m_sessionMapMutex);
	for (auto& pair : m_sessionMap) func(pair.second);
}

unsigned SessionManager::CountReadySessions() const
{
	return 0;
}

void SessionManager::ResetAllReadyFlags() {}

void SessionManager::Clear()
{
	Mutex::Lock lock(m_sessionMapMutex);
	m_sessionMap.clear();
}

unsigned SessionManager::GetRawSessionCount() const
{
	Mutex::Lock lock(m_sessionMapMutex);
	return (unsigned)m_sessionMap.size();
}

unsigned SessionManager::GetSessionCountWithState(int state) const
{
	return 0;
}

bool SessionManager::HasSessionWithState(int state) const
{
	return false;
}

void SessionManager::SendToSession(SenderHelper &sender, SessionId id, std::shared_ptr<NetPacket> packet)
{
	auto session = GetSessionById(id);
	if (session) sender.Send(session, packet);
}

void SessionManager::SendToAllSessions(SenderHelper &sender, std::shared_ptr<NetPacket> packet, int state)
{
	Mutex::Lock lock(m_sessionMapMutex);
	for (auto& pair : m_sessionMap) sender.Send(pair.second, packet);
}

void SessionManager::SendLobbyMsgToAllSessions(SenderHelper &sender, std::shared_ptr<NetPacket> packet, int state)
{
	SendToAllSessions(sender, packet, state);
}

void SessionManager::SendToAllButOneSessions(SenderHelper &sender, std::shared_ptr<NetPacket> packet, SessionId except, int state)
{
	Mutex::Lock lock(m_sessionMapMutex);
	for (auto& pair : m_sessionMap) {
		if (pair.first != except) sender.Send(pair.second, packet);
	}
}

END_UPP_NAMESPACE
