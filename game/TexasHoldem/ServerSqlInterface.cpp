#include "ServerSqlInterface.h"
#include <plugin/sqlite3/Sqlite3.h>

NAMESPACE_UPP

ServerSqlInterface::ServerSqlInterface(ServerDBCallback &cb)
	: m_callback(cb)
{
}

ServerSqlInterface::~ServerSqlInterface()
{
	Stop();
}

void ServerSqlInterface::Init(const String &host, const String &user, const String &pwd,
				  const String &database, const String &encryptionKey)
{
	m_host = host;
	m_user = user;
	m_pwd = pwd;
	m_database = database;
	m_encryptionKey = encryptionKey;
}

void ServerSqlInterface::Start()
{
	Run();
}

void ServerSqlInterface::Stop()
{
	SignalTermination();
}

void ServerSqlInterface::AsyncPlayerLogin(unsigned requestId, const String &playerName)
{
	Mutex::Lock lock(m_requestMutex);
	auto& r = m_requests.Add();
	r.requestId = requestId;
	r.type = REQ_LOGIN;
	r.name = playerName;
}

void ServerSqlInterface::AsyncCheckAvatarBlacklist(unsigned requestId, const String &avatarHash)
{
	Mutex::Lock lock(m_requestMutex);
	auto& r = m_requests.Add();
	r.requestId = requestId;
	r.type = REQ_AVATAR_BLACKLIST;
	r.data1 = avatarHash;
}

void ServerSqlInterface::PlayerPostLogin(DB_id playerId, const String &avatarHash, const String &avatarType)
{
}

void ServerSqlInterface::PlayerLogout(DB_id playerId)
{
}

void ServerSqlInterface::AsyncCreateGame(unsigned requestId, const String &gameName)
{
	Mutex::Lock lock(m_requestMutex);
	auto& r = m_requests.Add();
	r.requestId = requestId;
	r.type = REQ_CREATE_GAME;
	r.name = gameName;
}

void ServerSqlInterface::SetGamePlayerPlace(unsigned requestId, DB_id playerId, unsigned place)
{
}

void ServerSqlInterface::SetPlayerLastGames(unsigned requestId, DB_id playerId, const std::vector<long>& last_games, String playerIp)
{
}

void ServerSqlInterface::EndGame(unsigned requestId)
{
}

void ServerSqlInterface::AsyncReportAvatar(unsigned requestId, unsigned replyId, DB_id reportedPlayerId, const String &avatarHash, const String &avatarType, DB_id *byPlayerId)
{
}

void ServerSqlInterface::AsyncReportGame(unsigned requestId, unsigned replyId, DB_id *creatorPlayerId, unsigned gameId, const String &gameName, DB_id *byPlayerId)
{
}

void ServerSqlInterface::AsyncQueryAdminPlayers(unsigned requestId)
{
}

void ServerSqlInterface::AsyncBlockPlayer(unsigned requestId, unsigned replyId, DB_id playerId, int valid, int active)
{
}

void ServerSqlInterface::Main()
{
	Sqlite3Session sqlite;
	if (!sqlite.Open(m_database)) {
		return;
	}
	Sql sql(sqlite);

	while (!ShouldTerminate()) {
		Vector<AsyncRequest> reqs;
		{
			Mutex::Lock lock(m_requestMutex);
			reqs = std::move(m_requests);
			m_requests.Clear();
		}

		for (auto& r : reqs) {
			switch (r.type) {
			case REQ_LOGIN:
				sql * Select(SqlAll()).From(SqlId("players")).Where(SqlId("name") == r.name);
				if (sql.Fetch()) {
					auto p = std::make_shared<DBPlayerData>();
					p->id = (int)sql[0];
					m_callback.PlayerLoginSuccess(r.requestId, p);
				} else {
					m_callback.PlayerLoginFailed(r.requestId);
				}
				break;
			case REQ_AVATAR_BLACKLIST:
				m_callback.AvatarIsOK(r.requestId);
				break;
			case REQ_CREATE_GAME:
				m_callback.CreateGameSuccess(r.requestId);
				break;
			}
		}
		Msleep(50);
	}
}

END_UPP_NAMESPACE
