#ifndef _CardEngine_SessionManager_h_
#define _CardEngine_SessionManager_h_

#include <memory>
#include <map>
#include <vector>
#include <functional>
#include "SessionData.h"
#include <GameRules/PlayerData.h>
#include <GameRules/GameData.h>

NAMESPACE_UPP

class NetPacket;
class SenderHelper;

class SessionManager
{
public:
	SessionManager();
	virtual ~SessionManager();

	void AddSession(std::shared_ptr<SessionData> sessionData);
	void SetSessionPlayerData(SessionId session, std::shared_ptr<PlayerData> playerData);
	bool RemoveSession(SessionId session);

	std::shared_ptr<SessionData> GetSessionById(SessionId id) const;
	std::shared_ptr<SessionData> GetSessionByPlayerName(const String &playerName) const;
	std::shared_ptr<SessionData> GetSessionByUniquePlayerId(unsigned uniqueId, bool initSessions = false) const;
	std::vector<std::shared_ptr<SessionData>> GetAllSessions() const;

	PlayerDataList GetPlayerDataList() const;
	PlayerDataList GetSpectatorDataList() const;
	PlayerIdList GetPlayerIdList(int state) const;
	bool IsPlayerConnected(const String &playerName) const;
	bool IsPlayerConnected(unsigned uniqueId) const;
	bool IsClientAddressConnected(const String &clientAddress) const;
	bool IsGuestAllowedToConnect(const String &clientAddress) const;
	
	void ForEach(std::function<void (std::shared_ptr<SessionData>)> func);

	unsigned CountReadySessions() const;
	void ResetAllReadyFlags();

	void Clear();
	unsigned GetRawSessionCount() const;
	unsigned GetSessionCountWithState(int state) const;
	bool HasSessionWithState(int state) const;
	unsigned GetPlayerCount() const { return GetRawSessionCount(); }

	void SendToSession(SenderHelper &sender, SessionId id, std::shared_ptr<NetPacket> packet);
	void SendToAllSessions(SenderHelper &sender, std::shared_ptr<NetPacket> packet, int state);
	void SendLobbyMsgToAllSessions(SenderHelper &sender, std::shared_ptr<NetPacket> packet, int state);
	void SendToAllButOneSessions(SenderHelper &sender, std::shared_ptr<NetPacket> packet, SessionId except, int state);

protected:
	typedef std::map<SessionId, std::shared_ptr<SessionData>> SessionMap;

private:
	SessionMap m_sessionMap;
	mutable Mutex m_sessionMapMutex;
};

END_UPP_NAMESPACE

#endif
