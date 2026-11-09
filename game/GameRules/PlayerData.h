#ifndef _CardEngine_PlayerData_h_
#define _CardEngine_PlayerData_h_

#include <Core/Core.h>
#include <memory>
#include <vector>
#include <map>
#include <GameRules/GameDefs.h>
#include <EditorCommon/CryptHelper.h>

NAMESPACE_UPP

enum PlayerType {
	PLAYER_TYPE_COMPUTER,
	PLAYER_TYPE_HUMAN
};

enum PlayerRights {
	PLAYER_RIGHTS_GUEST = 1,
	PLAYER_RIGHTS_NORMAL,
	PLAYER_RIGHTS_ADMIN
};

enum AvatarFileType {
	AVATAR_FILE_TYPE_UNKNOWN = 0,
	AVATAR_FILE_TYPE_PNG,
	AVATAR_FILE_TYPE_JPG,
	AVATAR_FILE_TYPE_GIF
};

struct AvatarFile {
	AvatarFile() : fileType(AVATAR_FILE_TYPE_UNKNOWN), reportedSize(0) {}
	Vector<byte>	fileData;
	AvatarFileType	fileType;
	int64			reportedSize;

	void Serialize(Stream& s) {
		s % fileData;
		int ft = (int)fileType;
		s % ft; fileType = (AvatarFileType)ft;
		s % reportedSize;
	}
};

struct PlayerInfo {
	PlayerInfo() : ptype(PLAYER_TYPE_HUMAN), isGuest(false), isAdmin(false), hasAvatar(false), avatarType(AVATAR_FILE_TYPE_UNKNOWN) {}
	String			playerName;
	PlayerType		ptype;
	bool			isGuest;
	bool			isAdmin;
	String			countryCode;
	bool			hasAvatar;
	MD5Buf			avatar;
	AvatarFileType	avatarType;

	void Serialize(Stream& s) {
		s % playerName;
		int pt = (int)ptype;
		s % pt; ptype = (PlayerType)pt;
		s % isGuest % isAdmin % countryCode % hasAvatar % avatar;
		int at = (int)avatarType;
		s % at; avatarType = (AvatarFileType)at;
	}
};

class PlayerData
{
public:
	PlayerData(unsigned uniqueId, int number, PlayerType type, PlayerRights rights, bool isGameAdmin);
	PlayerData(const PlayerData &other);
	~PlayerData();

	String GetName() const;
	void SetName(const String &name);
	String GetCountry() const;
	void SetCountry(const String &country);
	String GetAvatarFile() const;
	void SetAvatarFile(const String &avatarFile);
	MD5Buf GetAvatarMD5() const;
	void SetAvatarMD5(const MD5Buf &avatarMD5);
	std::shared_ptr<AvatarFile> GetNetAvatarFile() const;
	void SetNetAvatarFile(std::shared_ptr<AvatarFile> AvatarFile);
	PlayerType GetType() const;
	void SetType(PlayerType type);
	PlayerRights GetRights() const;
	void SetRights(PlayerRights rights);
	bool IsGameAdmin() const;
	void SetGameAdmin(bool isAdmin);
	unsigned GetUniqueId() const;
	int GetNumber() const;
	void SetNumber(int number);
	String GetGuid() const;
	void SetGuid(const String &guid);
	String GetOldGuid() const;
	void SetOldGuid(const String &guid);
	DB_id GetDBId() const;
	void SetDBId(DB_id id);
	int GetStartCash() const;
	void SetStartCash(int cash);

	void AddPlayerLastGame(long last_games);
	void SetPlayerLastGames(Vector<long> last_games);
	Vector<long> GetPlayerLastGames();
	bool IsPlayerAllowedToJoinCreateLimitRank(String num, String period);

	bool operator<(const PlayerData &other) const;

	void Serialize(Stream& s);

private:
	const unsigned					m_uniqueId;
	DB_id							m_dbId;
	int								m_number;
	int								m_startCash; // only used if > 0
	String							m_guid;
	String							m_oldGuid;
	String							m_name;
	String							m_password;
	String							m_country;
	String							m_avatarFile;
	MD5Buf							m_avatarMD5;
	PlayerType						m_type;
	PlayerRights					m_rights;
	bool							m_isGameAdmin;
	std::shared_ptr<AvatarFile>	m_netAvatarFile;

	Vector<long> 				m_last_games;

	mutable Mutex				m_dataMutex;
};

typedef std::vector<std::pair<unsigned, unsigned>> RemovePlayerList;
typedef std::vector<std::shared_ptr<PlayerData>> PlayerDataList;
typedef std::map<unsigned, std::shared_ptr<PlayerData>> PlayerDataMap;

END_UPP_NAMESPACE

#endif
