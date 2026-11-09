#include "ServerBanManager.h"
#include <CtrlLib/CtrlLib.h>
#include <plugin/pcre/Pcre.h>

NAMESPACE_UPP

ServerBanManager::ServerBanManager()
	: m_curBanId(0)
{
}

ServerBanManager::~ServerBanManager()
{
}

void ServerBanManager::SetAdminPlayerIds(const Vector<DB_id> &adminList)
{
	Mutex::Lock lock(m_banMutex);
	m_adminPlayers.SetCount(adminList.GetCount());
	for (int i = 0; i < adminList.GetCount(); i++) m_adminPlayers[i] = adminList[i];
	Sort(m_adminPlayers);
}

void ServerBanManager::BanPlayerName(const String &playerName, unsigned durationHours)
{
	Mutex::Lock lock(m_banMutex);
	unsigned banId = GetNextBanId();
	TimedPlayerBan& b = m_banPlayerNameMap.Add(banId);
	b.nameStr = playerName;
	if (durationHours > 0) {
		b.expiry = GetSysTime() + (int64)durationHours * 3600;
		InternalRegisterTimedBan(banId, durationHours);
	}
}

void ServerBanManager::BanPlayerRegex(const String &playerRegex, unsigned durationHours)
{
	Mutex::Lock lock(m_banMutex);
	unsigned banId = GetNextBanId();
	TimedPlayerBan& b = m_banPlayerNameMap.Add(banId);
	b.regexStr = playerRegex;
	if (durationHours > 0) {
		b.expiry = GetSysTime() + (int64)durationHours * 3600;
		InternalRegisterTimedBan(banId, durationHours);
	}
}

void ServerBanManager::BanIPAddress(const String &ipAddress, unsigned durationHours)
{
	Mutex::Lock lock(m_banMutex);
	unsigned banId = GetNextBanId();
	TimedIPBan& b = m_banIPAddressMap.Add(banId);
	b.ipAddress = ipAddress;
	if (durationHours > 0) {
		b.expiry = GetSysTime() + (int64)durationHours * 3600;
		InternalRegisterTimedBan(banId, durationHours);
	}
}

bool ServerBanManager::UnBan(unsigned banId)
{
	Mutex::Lock lock(m_banMutex);
	int idx = m_banPlayerNameMap.Find(banId);
	if (idx >= 0) {
		m_banPlayerNameMap.Remove(idx);
		KillTimeCallback((void*)(uintptr_t)banId);
		return true;
	}
	idx = m_banIPAddressMap.Find(banId);
	if (idx >= 0) {
		m_banIPAddressMap.Remove(idx);
		KillTimeCallback((void*)(uintptr_t)banId);
		return true;
	}
	return false;
}

void ServerBanManager::GetBanList(Vector<String> &list) const
{
	Mutex::Lock lock(m_banMutex);
	for (int i = 0; i < m_banPlayerNameMap.GetCount(); i++) {
		const auto& b = m_banPlayerNameMap[i];
		String s = Format("%d: ", (int)m_banPlayerNameMap.GetKey(i));
		if (b.nameStr.GetCount()) s << "(nickStr) - " << b.nameStr;
		else s << "(nickRegex) - " << b.regexStr;
		if (!IsNull(b.expiry)) s << " expires: " << b.expiry;
		list.Add(s);
	}
	for (int i = 0; i < m_banIPAddressMap.GetCount(); i++) {
		const auto& b = m_banIPAddressMap[i];
		String s = Format("%d: (IP) - %s", (int)m_banIPAddressMap.GetKey(i), b.ipAddress);
		if (!IsNull(b.expiry)) s << " expires: " << b.expiry;
		list.Add(s);
	}
}

void ServerBanManager::ClearBanList()
{
	Mutex::Lock lock(m_banMutex);
	for (int i = 0; i < m_banPlayerNameMap.GetCount(); i++) KillTimeCallback((void*)(uintptr_t)m_banPlayerNameMap.GetKey(i));
	for (int i = 0; i < m_banIPAddressMap.GetCount(); i++) KillTimeCallback((void*)(uintptr_t)m_banIPAddressMap.GetKey(i));
	m_banPlayerNameMap.Clear();
	m_banIPAddressMap.Clear();
}

bool ServerBanManager::IsAdminPlayer(DB_id playerId) const
{
	if (playerId == DB_ID_INVALID) return false;
	Mutex::Lock lock(m_banMutex);
	return FindIndex(m_adminPlayers, playerId) >= 0;
}

bool ServerBanManager::IsPlayerBanned(const String &name) const
{
	Mutex::Lock lock(m_banMutex);
	for (int i = 0; i < m_banPlayerNameMap.GetCount(); i++) {
		const auto& b = m_banPlayerNameMap[i];
		if (b.nameStr.GetCount()) {
			if (b.nameStr == name) return true;
		} else if (b.regexStr.GetCount()) {
			RegExp re(b.regexStr, RegExp::UNICODE | RegExp::CASELESS);
			if (re.Match(name)) return true;
		}
	}
	return false;
}

bool ServerBanManager::IsIPAddressBanned(const String &ipAddress) const
{
	Mutex::Lock lock(m_banMutex);
	for (int i = 0; i < m_banIPAddressMap.GetCount(); i++) {
		if (m_banIPAddressMap[i].ipAddress == ipAddress) return true;
	}
	return false;
}

void ServerBanManager::InitGameNameBadWordList(const Vector<String> &badWordList)
{
	Mutex::Lock lock(m_banMutex);
	m_gameNameBadWordFilter <<= badWordList;
}

bool ServerBanManager::IsBadGameName(const String &name) const
{
	Mutex::Lock lock(m_banMutex);
	for (const String& rs : m_gameNameBadWordFilter) {
		RegExp re(rs, RegExp::UNICODE | RegExp::CASELESS);
		if (re.Match(name)) return true;
	}
	return false;
}

void ServerBanManager::InternalRegisterTimedBan(unsigned banId, unsigned durationHours)
{
	SetTimeCallback((int)durationHours * 3600 * 1000, [this, banId] { TimerRemoveBan(banId); }, (void*)(uintptr_t)banId);
}

void ServerBanManager::TimerRemoveBan(unsigned banId)
{
	UnBan(banId);
}

unsigned ServerBanManager::GetNextBanId()
{
	m_curBanId++;
	if (m_curBanId == 0) m_curBanId++;
	return m_curBanId;
}

END_UPP_NAMESPACE
