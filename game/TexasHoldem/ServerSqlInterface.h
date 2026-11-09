#ifndef _CardEngine_ServerSqlInterface_h_
#define _CardEngine_ServerSqlInterface_h_

#include "DbInterface.h"
#include "Thread.h"
#include <Sql/Sql.h>

NAMESPACE_UPP

class ServerSqlInterface : public ServerDBInterface, public ThreadWrapper
{
public:
	ServerSqlInterface(ServerDBCallback &cb);
	virtual ~ServerSqlInterface();

	virtual void Init(const String &host, const String &user, const String &pwd,
					  const String &database, const String &encryptionKey) override;

	virtual void Start() override;
	virtual void Stop() override;

	virtual void AsyncPlayerLogin(unsigned requestId, const String &playerName) override;
	virtual void AsyncCheckAvatarBlacklist(unsigned requestId, const String &avatarHash) override;
	virtual void PlayerPostLogin(DB_id playerId, const String &avatarHash, const String &avatarType) override;
	virtual void PlayerLogout(DB_id playerId) override;

	virtual void AsyncCreateGame(unsigned requestId, const String &gameName) override;
	virtual void SetGamePlayerPlace(unsigned requestId, DB_id playerId, unsigned place) override;
	virtual void SetPlayerLastGames(unsigned requestId, DB_id playerId, const std::vector<long>& last_games, String playerIp) override;
	virtual void EndGame(unsigned requestId) override;

	virtual void AsyncReportAvatar(unsigned requestId, unsigned replyId, DB_id reportedPlayerId, const String &avatarHash, const String &avatarType, DB_id *byPlayerId) override;
	virtual void AsyncReportGame(unsigned requestId, unsigned replyId, DB_id *creatorPlayerId, unsigned gameId, const String &gameName, DB_id *byPlayerId) override;

	virtual void AsyncQueryAdminPlayers(unsigned requestId) override;
	virtual void AsyncBlockPlayer(unsigned requestId, unsigned replyId, DB_id playerId, int valid, int active) override;

protected:
	virtual void Main() override;

private:
	ServerDBCallback &m_callback;
	
	String m_host, m_user, m_pwd, m_database, m_encryptionKey;
	
	struct AsyncRequest : Moveable<AsyncRequest> {
		unsigned requestId;
		unsigned replyId;
		int type;
		String name;
		String data1;
		String data2;
		DB_id dbid1;
		DB_id dbid2;
		Vector<long> last_games;
		unsigned val1;
	};
	
	enum RequestType {
		REQ_LOGIN,
		REQ_AVATAR_BLACKLIST,
		REQ_CREATE_GAME,
		REQ_QUERY_ADMINS,
		REQ_BLOCK_PLAYER,
		REQ_REPORT_AVATAR,
		REQ_REPORT_GAME
	};

	Vector<AsyncRequest> m_requests;
	Mutex m_requestMutex;
};

END_UPP_NAMESPACE

#endif
