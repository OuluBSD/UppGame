#include "AvatarManager.h"
#include <Core/Core.h>

NAMESPACE_UPP

AvatarManager::AvatarManager(bool useExternalServer, const String &externalServerAddress,
							  const String &externalServerUser, const String &externalServerPassword)
	: m_useExternalServer(useExternalServer), m_externalServerAddress(externalServerAddress),
	  m_externalServerUser(externalServerUser), m_externalServerPassword(externalServerPassword)
{}

AvatarManager::~AvatarManager() {}

bool AvatarManager::Init(const String &dataDir, const String &cacheDir)
{
	Mutex::Lock lock(m_cacheDirMutex);
	m_cacheDir = cacheDir;
	RealizeDirectory(m_cacheDir);
	
	InternalReadDirectory(dataDir, m_avatars);
	InternalReadDirectory(cacheDir, m_cachedAvatars);
	
	return true;
}

bool AvatarManager::AddSingleAvatar(const String &fileName)
{
	MD5Buf hash;
	if (GetHashForAvatar(fileName, hash)) {
		Mutex::Lock lock(m_avatarsMutex);
		m_avatars[hash] = fileName;
		return true;
	}
	return false;
}

std::shared_ptr<AvatarFileState> AvatarManager::OpenAvatarFileForChunkRead(const String &fileName, unsigned &outFileSize, AvatarFileType &outFileType)
{
	// Implementation for chunked reading during network transfer
	return nullptr;
}

unsigned AvatarManager::ChunkReadAvatarFile(std::shared_ptr<AvatarFileState> fileState, byte *data, unsigned chunkSize)
{
	return 0;
}

AvatarFileType AvatarManager::GetAvatarFileType(const String &fileName)
{
	String ext = ToLower(GetFileExt(fileName));
	if (ext == ".png") return AVATAR_FILE_TYPE_PNG;
	if (ext == ".jpg" || ext == ".jpeg") return AVATAR_FILE_TYPE_JPG;
	if (ext == ".gif") return AVATAR_FILE_TYPE_GIF;
	return AVATAR_FILE_TYPE_UNKNOWN;
}

String AvatarManager::GetAvatarFileExtension(AvatarFileType fileType)
{
	switch (fileType) {
		case AVATAR_FILE_TYPE_PNG: return ".png";
		case AVATAR_FILE_TYPE_JPG: return ".jpg";
		case AVATAR_FILE_TYPE_GIF: return ".gif";
		default: return "";
	}
}

bool AvatarManager::GetHashForAvatar(const String &fileName, MD5Buf &md5buf) const
{
	String data = LoadFile(fileName);
	if (data.IsEmpty()) return false;
	
	::Upp::MD5(md5buf.GetData(), ~data, (dword)data.GetCount());
	return true;
}

bool AvatarManager::GetAvatarFileName(const MD5Buf &md5buf, String &fileName) const
{
	{
		Mutex::Lock lock(m_avatarsMutex);
		auto it = m_avatars.find(md5buf);
		if (it != m_avatars.end()) { fileName = it->second; return true; }
	}
	{
		Mutex::Lock lock(m_cachedAvatarsMutex);
		auto it = m_cachedAvatars.find(md5buf);
		if (it != m_cachedAvatars.end()) { fileName = it->second; return true; }
	}
	return false;
}

bool AvatarManager::HasAvatar(const MD5Buf &md5buf) const
{
	String dummy;
	return GetAvatarFileName(md5buf, dummy);
}

bool AvatarManager::StoreAvatarInCache(const MD5Buf &md5buf, AvatarFileType avatarFileType, const byte *data, size_t size, bool upload)
{
	String hashStr = HexString(md5buf.GetData(), 16);
	String fileName;
	{
		Mutex::Lock lock(m_cacheDirMutex);
		fileName = AppendFileName(m_cacheDir, hashStr + GetAvatarFileExtension(avatarFileType));
	}
	
	if (SaveFile(fileName, String(data, (int)size))) {
		Mutex::Lock lock(m_cachedAvatarsMutex);
		m_cachedAvatars[md5buf] = fileName;
		return true;
	}
	return false;
}

bool AvatarManager::IsValidAvatarFileType(AvatarFileType avatarFileType, const byte *fileHeader, size_t fileHeaderSize)
{
	if (fileHeaderSize < 4) return false;
	if (avatarFileType == AVATAR_FILE_TYPE_PNG && memcmp(fileHeader, "\x89PNG", 4) == 0) return true;
	if (avatarFileType == AVATAR_FILE_TYPE_JPG && memcmp(fileHeader, "\xFF\xD8\xFF", 3) == 0) return true;
	if (avatarFileType == AVATAR_FILE_TYPE_GIF && memcmp(fileHeader, "GIF8", 4) == 0) return true;
	return false;
}

bool AvatarManager::InternalReadDirectory(const String &dir, AvatarMap &avatars)
{
	FindFile ff(AppendFileName(dir, "*.*"));
	while (ff) {
		if (ff.IsFile()) {
			MD5Buf hash;
			String path = AppendFileName(dir, ff.GetName());
			if (GetHashForAvatar(path, hash)) {
				avatars[hash] = path;
			}
		}
		ff.Next();
	}
	return true;
}

END_UPP_NAMESPACE