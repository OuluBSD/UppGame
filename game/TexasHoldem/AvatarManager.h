#ifndef _CardEngine_AvatarManager_h_
#define _CardEngine_AvatarManager_h_

#include <memory>
#include <map>
#include <vector>
#include <GameRules/PlayerData.h>
#include <EditorCommon/CryptHelper.h>
#include "NetPacket.h"

NAMESPACE_UPP

#define MIN_AVATAR_FILE_SIZE    32
#define MAX_AVATAR_FILE_SIZE    30720

struct AvatarFileState;
class UploaderThread;

class AvatarManager
{
public:
	AvatarManager(bool useExternalServer = false, const String &externalServerAddress = "",
				  const String &externalServerUser = "", const String &externalServerPassword = "");
	~AvatarManager();

	bool Init(const String &dataDir, const String &cacheDir);

	bool AddSingleAvatar(const String &fileName);

	static std::shared_ptr<AvatarFileState> OpenAvatarFileForChunkRead(const String &fileName, unsigned &outFileSize, AvatarFileType &outFileType);
	static unsigned ChunkReadAvatarFile(std::shared_ptr<AvatarFileState> fileState, byte *data, unsigned chunkSize);

	static int AvatarFileToNetPackets(const String &fileName, unsigned requestId, NetPacketList &packets);
	static AvatarFileType GetAvatarFileType(const String &fileName);
	static String GetAvatarFileExtension(AvatarFileType fileType);

	bool GetHashForAvatar(const String &fileName, MD5Buf &md5buf) const;
	bool GetAvatarFileName(const MD5Buf &md5buf, String &fileName) const;
	bool HasAvatar(const MD5Buf &md5buf) const;
	bool StoreAvatarInCache(const MD5Buf &md5buf, AvatarFileType avatarFileType, const byte *data, size_t size, bool upload);

	static bool IsValidAvatarFileType(AvatarFileType avatarFileType, const byte *fileHeader, size_t fileHeaderSize);

	void RemoveOldAvatarCacheEntries();

protected:
	typedef std::map<MD5Buf, String> AvatarMap;
	
	bool InternalReadDirectory(const String &dir, AvatarMap &avatars);

private:
	mutable Mutex m_avatarsMutex;
	AvatarMap m_avatars;

	mutable Mutex m_cachedAvatarsMutex;
	AvatarMap m_cachedAvatars;

	mutable Mutex m_cacheDirMutex;
	String m_cacheDir;

	const bool m_useExternalServer;
	const String m_externalServerAddress;
	const String m_externalServerUser;
	const String m_externalServerPassword;

	std::shared_ptr<UploaderThread> m_uploader;
};

END_UPP_NAMESPACE

#endif
