#include <GameRules/PlayerData.h>
#include <ctime>

NAMESPACE_UPP

PlayerData::PlayerData(unsigned uniqueId, int number, PlayerType type, PlayerRights rights, bool isGameAdmin)
	: m_uniqueId(uniqueId), m_dbId(DB_ID_INVALID), m_number(number), m_startCash(-1), m_type(type), m_rights(rights), m_isGameAdmin(isGameAdmin)
{
}

PlayerData::PlayerData(const PlayerData &other)
	: m_uniqueId(other.GetUniqueId()), m_dbId(other.GetDBId()), m_number(other.GetNumber()), m_startCash(other.GetStartCash()),
	  m_guid(other.GetGuid()), m_oldGuid(other.GetOldGuid()), m_name(other.GetName()), m_password(), m_country(other.GetCountry()),
	  m_avatarFile(other.GetAvatarFile()), m_avatarMD5(other.GetAvatarMD5()), m_type(other.GetType()), m_rights(other.GetRights()),
	  m_isGameAdmin(other.IsGameAdmin()), m_netAvatarFile(), m_dataMutex()
{
}

PlayerData::~PlayerData()
{
}

String PlayerData::GetName() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_name;
}

void PlayerData::SetName(const String &name)
{
	Mutex::Lock lock(m_dataMutex);
	m_name = name;
}

String PlayerData::GetCountry() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_country;
}

void PlayerData::SetCountry(const String &country)
{
	Mutex::Lock lock(m_dataMutex);
	m_country = country;
}

String PlayerData::GetAvatarFile() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_avatarFile;
}

void PlayerData::SetAvatarFile(const String &avatarFile)
{
	Mutex::Lock lock(m_dataMutex);
	m_avatarFile = avatarFile;
}

MD5Buf PlayerData::GetAvatarMD5() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_avatarMD5;
}

void PlayerData::SetAvatarMD5(const MD5Buf &avatarMD5)
{
	Mutex::Lock lock(m_dataMutex);
	m_avatarMD5 = avatarMD5;
}

std::shared_ptr<AvatarFile> PlayerData::GetNetAvatarFile() const
{
	return m_netAvatarFile;
}

void PlayerData::SetNetAvatarFile(std::shared_ptr<AvatarFile> avatarFile)
{
	m_netAvatarFile = avatarFile;
}

PlayerType PlayerData::GetType() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_type;
}

void PlayerData::SetType(PlayerType type)
{
	Mutex::Lock lock(m_dataMutex);
	m_type = type;
}

PlayerRights PlayerData::GetRights() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_rights;
}

void PlayerData::SetRights(PlayerRights rights)
{
	Mutex::Lock lock(m_dataMutex);
	m_rights = rights;
}

bool PlayerData::IsGameAdmin() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_isGameAdmin;
}

void PlayerData::SetGameAdmin(bool isAdmin)
{
	Mutex::Lock lock(m_dataMutex);
	m_isGameAdmin = isAdmin;
}

unsigned PlayerData::GetUniqueId() const
{
	return m_uniqueId;
}

int PlayerData::GetNumber() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_number;
}

void PlayerData::SetNumber(int number)
{
	Mutex::Lock lock(m_dataMutex);
	m_number = number;
}

String PlayerData::GetGuid() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_guid;
}

void PlayerData::SetGuid(const String &guid)
{
	Mutex::Lock lock(m_dataMutex);
	m_guid = guid;
}

String PlayerData::GetOldGuid() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_oldGuid;
}

void PlayerData::SetOldGuid(const String &guid)
{
	Mutex::Lock lock(m_dataMutex);
	m_oldGuid = guid;
}

DB_id PlayerData::GetDBId() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_dbId;
}

void PlayerData::SetDBId(DB_id id)
{
	Mutex::Lock lock(m_dataMutex);
	m_dbId = id;
}

int PlayerData::GetStartCash() const
{
	Mutex::Lock lock(m_dataMutex);
	return m_startCash;
}

void PlayerData::SetStartCash(int cash)
{
	Mutex::Lock lock(m_dataMutex);
	m_startCash = cash;
}

bool PlayerData::operator<(const PlayerData &other) const
{
	Mutex::Lock lock(m_dataMutex);
	return m_number < other.GetNumber();
}

void PlayerData::SetPlayerLastGames(Vector<long> last_games)
{
	Mutex::Lock lock(m_dataMutex);
	m_last_games = pick(last_games);
}

void PlayerData::AddPlayerLastGame(long lastGame)
{
	Mutex::Lock lock(m_dataMutex);
	m_last_games.Add(lastGame);
}

Vector<long> PlayerData::GetPlayerLastGames()
{
	Mutex::Lock lock(m_dataMutex);
	Vector<long> res;
	res.Append(m_last_games);
	return res;
}

bool PlayerData::IsPlayerAllowedToJoinCreateLimitRank(String num, String period)
{
	Mutex::Lock lock(m_dataMutex);
	long then = (long)time(NULL) - (long)(ScanInt(period) * 60);

	int count = 0;
	for(int i = 0; i < m_last_games.GetCount(); ) {
		if(m_last_games[i] > then) {
			count++;
			++i;
		} else {
			m_last_games.Remove(i);
		}
	}

	if(count < ScanInt(num))
		return true;

	return false;
}

void PlayerData::Serialize(Stream& s)
{
	Mutex::Lock lock(m_dataMutex);
	s % m_dbId % m_number % m_startCash % m_guid % m_oldGuid % m_name % m_password % m_country % m_avatarFile % m_avatarMD5;
	int pt = (int)m_type;
	s % pt; m_type = (PlayerType)pt;
	int pr = (int)m_rights;
	s % pr; m_rights = (PlayerRights)pr;
	s % m_isGameAdmin % m_last_games;
	// skipping m_netAvatarFile for now as shared_ptr serialization is complex in U++
}

END_UPP_NAMESPACE
