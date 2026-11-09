#ifndef _CardEngine_DbInterface_h_
#define _CardEngine_DbInterface_h_

#include <memory>
#include <vector>
#include <GameRules/GameDefs.h>

NAMESPACE_UPP

struct DBPlayerData {
	DBPlayerData() : id(DB_ID_INVALID) {}
	DB_id id;
	String secret;
	String country;
	String last_login;
	String last_games;
	String last_ip;
};

class ServerDBCallback
{
public:
	virtual ~ServerDBCallback() {}

	virtual void ConnectSuccess() = 0;
	virtual void ConnectFailed(String error) = 0;

	virtual void QueryError(String error) = 0;

	virtual void PlayerLoginSuccess(unsigned requestId, std::shared_ptr<DBPlayerData> dbPlayerData) = 0;
	virtual void PlayerLoginFailed(unsigned requestId) = 0;
	virtual void PlayerLoginBlocked(unsigned requestId) = 0;

	virtual void AvatarIsBlacklisted(unsigned requestId) = 0;
	virtual void AvatarIsOK(unsigned requestId) = 0;

	virtual void CreateGameSuccess(unsigned requestId) = 0;
	virtual void CreateGameFailed(unsigned requestId) = 0;

	virtual void ReportAvatarSuccess(unsigned requestId, unsigned replyId) = 0;
	virtual void ReportAvatarFailed(unsigned requestId, unsigned replyId) = 0;

	virtual void ReportGameSuccess(unsigned requestId, unsigned replyId) = 0;
	virtual void ReportGameFailed(unsigned requestId, unsigned replyId) = 0;

	virtual void PlayerAdminList(unsigned requestId, const Vector<DB_id>& adminList) = 0;

	virtual void BlockPlayerSuccess(unsigned requestId, unsigned replyId) = 0;
	virtual void BlockPlayerFailed(unsigned requestId, unsigned replyId) = 0;
};

class ServerDBInterface
{
public:
	virtual ~ServerDBInterface() {}

	virtual void Init(const String &host, const String &user, const String &pwd,
					  const String &database, const String &encryptionKey) = 0;

	virtual void Start() = 0;
	virtual void Stop() = 0;

	virtual void AsyncPlayerLogin(unsigned requestId, const String &playerName) = 0;
	virtual void AsyncCheckAvatarBlacklist(unsigned requestId, const String &avatarHash) = 0;
	virtual void PlayerPostLogin(DB_id playerId, const String &avatarHash, const String &avatarType) = 0;
	virtual void PlayerLogout(DB_id playerId) = 0;

	virtual void AsyncCreateGame(unsigned requestId, const String &gameName) = 0;
	virtual void SetGamePlayerPlace(unsigned requestId, DB_id playerId, unsigned place) = 0;
	virtual void SetPlayerLastGames(unsigned requestId, DB_id playerId, const std::vector<long>& last_games, String playerIp) = 0;
	virtual void EndGame(unsigned requestId) = 0;

	virtual void AsyncReportAvatar(unsigned requestId, unsigned replyId, DB_id reportedPlayerId, const String &avatarHash, const String &avatarType, DB_id *byPlayerId) = 0;
	virtual void AsyncReportGame(unsigned requestId, unsigned replyId, DB_id *creatorPlayerId, unsigned gameId, const String &gameName, DB_id *byPlayerId) = 0;

	virtual void AsyncQueryAdminPlayers(unsigned requestId) = 0;
	virtual void AsyncBlockPlayer(unsigned requestId, unsigned replyId, DB_id playerId, int valid, int active) = 0;
};

class ServerDBNoAction : public ServerDBInterface
{
public:
	virtual void Init(const String &, const String &, const String &, const String &, const String &) override {}
	virtual void Start() override {}
	virtual void Stop() override {}
	virtual void AsyncPlayerLogin(unsigned, const String &) override {}
	virtual void AsyncCheckAvatarBlacklist(unsigned, const String &) override {}
	virtual void PlayerPostLogin(DB_id, const String &, const String &) override {}
	virtual void PlayerLogout(DB_id) override {}
	virtual void AsyncCreateGame(unsigned, const String &) override {}
	virtual void SetGamePlayerPlace(unsigned, DB_id, unsigned) override {}
	virtual void SetPlayerLastGames(unsigned, DB_id, const std::vector<long>&, String) override {}
	virtual void EndGame(unsigned) override {}
	virtual void AsyncReportAvatar(unsigned, unsigned, DB_id, const String &, const String &, DB_id *) override {}
	virtual void AsyncReportGame(unsigned, unsigned, DB_id *, unsigned, const String &, DB_id *) override {}
	virtual void AsyncQueryAdminPlayers(unsigned) override {}
	virtual void AsyncBlockPlayer(unsigned, unsigned, DB_id, int, int) override {}
};

class ServerDBFactory
{
public:
	virtual ~ServerDBFactory() {}
	virtual std::shared_ptr<ServerDBInterface> CreateServerDBInterface(ServerDBCallback *callback) = 0;
};

END_UPP_NAMESPACE

#endif
