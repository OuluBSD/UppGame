#ifndef _CardEngine_ServerBanManager_h_
#define _CardEngine_ServerBanManager_h_

#include <Core/Core.h>
#include <GameRules/GameDefs.h>

#include <memory>

NAMESPACE_UPP

class ServerBanManager : public std::enable_shared_from_this<ServerBanManager>
{
public:
	ServerBanManager();
	virtual ~ServerBanManager();

	void SetAdminPlayerIds(const Vector<DB_id> &adminList);

	void BanPlayerName(const String &playerName, unsigned durationHours = 0);
	void BanPlayerRegex(const String &playerRegex, unsigned durationHours = 0);
	void BanIPAddress(const String &ipAddress, unsigned durationHours);
	bool UnBan(unsigned banId);
	void GetBanList(Vector<String> &list) const;
	void ClearBanList();

	bool IsAdminPlayer(DB_id playerId) const;
	bool IsPlayerBanned(const String &name) const;
	bool IsIPAddressBanned(const String &ipAddress) const;

	void InitGameNameBadWordList(const Vector<String> &badWordList);
	bool IsBadGameName(const String &name) const;

protected:
	struct TimedPlayerBan : Moveable<TimedPlayerBan> {
		String nameStr;
		String regexStr;
		Time   expiry;
	};
	struct TimedIPBan : Moveable<TimedIPBan> {
		String ipAddress;
		Time   expiry;
	};

	typedef VectorMap<unsigned, TimedPlayerBan> RegexMap;
	typedef VectorMap<unsigned, TimedIPBan> IPAddressMap;
	typedef Vector<String> RegexList;

	void InternalRegisterTimedBan(unsigned banId, unsigned durationHours);
	void TimerRemoveBan(unsigned banId);

	unsigned GetNextBanId();

private:
	RegexMap m_banPlayerNameMap;
	RegexList m_gameNameBadWordFilter;
	IPAddressMap m_banIPAddressMap;
	Vector<DB_id> m_adminPlayers;
	unsigned m_curBanId;
	mutable Mutex m_banMutex;
};

END_UPP_NAMESPACE

#endif