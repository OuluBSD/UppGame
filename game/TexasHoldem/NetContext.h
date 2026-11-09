#ifndef _CardEngine_NetContext_h_
#define _CardEngine_NetContext_h_

#include <memory>
#include "SessionData.h"
#include <GameRules/PlayerData.h>

NAMESPACE_UPP

#ifndef _WIN32
#define SOCKET int
#endif

class NetPacket;
class ClientThread;
class ClientState;

class ClientContext
{
public:
	ClientContext();
	virtual ~ClientContext();

	std::shared_ptr<SessionData> GetSessionData() const { return m_sessionData; }
	void SetSessionData(std::shared_ptr<SessionData> sessionData) { m_sessionData = sessionData; }
	
	bool GetSctp() const { return m_sctp; }
	void SetSctp(bool sctp) { m_sctp = sctp; }
	bool GetTls() const { return m_tls; }
	void SetTls(bool tls) { m_tls = tls; }
	int GetAddrFamily() const { return m_addrFamily; }
	void SetAddrFamily(int addrFamily) { m_addrFamily = addrFamily; }
	
	String GetServerAddr() const { return m_serverAddr; }
	void SetServerAddr(const String &serverAddr) { m_serverAddr = serverAddr; }
	String GetServerPassword() const { return m_serverPassword; }
	void SetServerPassword(const String &serverPassword) { m_serverPassword = serverPassword; }
	String GetServerListUrl() const { return m_serverListUrl; }
	void SetServerListUrl(const String &serverListUrl) { m_serverListUrl = serverListUrl; }
	bool GetUseServerList() const { return m_useServerList; }
	void SetUseServerList(bool use) { m_useServerList = use; }
	unsigned GetServerPort() const { return m_serverPort; }
	void SetServerPort(unsigned serverPort) { m_serverPort = serverPort; }
	String GetAvatarServerAddr() const { return m_avatarServerAddr; }
	void SetAvatarServerAddr(const String &avatarServerAddr) { m_avatarServerAddr = avatarServerAddr; }
	String GetPassword() const { return m_password; }
	void SetPassword(const String &password) { m_password = password; }
	String GetPlayerName() const { return m_playerName; }
	void SetPlayerName(const String &playerName) { m_playerName = playerName; }
	PlayerRights GetPlayerRights() const { return m_playerRights; }
	void SetPlayerRights(PlayerRights rights) { m_playerRights = rights; }
	String GetAvatarFile() const { return m_avatarFile; }
	void SetAvatarFile(const String &avatarFile) { m_avatarFile = avatarFile; }
	String GetCacheDir() const { return m_cacheDir; }
	void SetCacheDir(const String &cacheDir) { m_cacheDir = cacheDir; }
	bool GetSubscribeLobbyMsg() const { return m_hasSubscribedLobbyMsg; }
	void SetSubscribeLobbyMsg(bool setSubscribe) { m_hasSubscribedLobbyMsg = setSubscribe; }
	String GetSessionGuid() const { return m_sessionGuid; }
	void SetSessionGuid(const String &sessionGuid) { m_sessionGuid = sessionGuid; }

private:
	std::shared_ptr<SessionData> m_sessionData;
	bool m_sctp = false;
	bool m_tls = false;
	int m_addrFamily = 0;
	String m_serverAddr;
	String m_serverPassword;
	String m_serverListUrl;
	bool m_useServerList = false;
	unsigned m_serverPort = 0;
	String m_avatarServerAddr;
	String m_password;
	String m_playerName;
	PlayerRights m_playerRights = PLAYER_RIGHTS_NORMAL;
	String m_avatarFile;
	String m_cacheDir;
	bool m_hasSubscribedLobbyMsg = false;
	String m_sessionGuid;
};

class NetContext
{
public:
	virtual ~NetContext() {}
	virtual SOCKET GetSocket() const = 0;
};

END_UPP_NAMESPACE

#endif