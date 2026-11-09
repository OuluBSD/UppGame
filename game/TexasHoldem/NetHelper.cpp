#include "NetHelper.h"

NAMESPACE_UPP

unsigned NetHelper::GetMaxNumberOfAvatarFiles(bool isServer)
{
	return isServer ? 2048 : 256;
}

unsigned NetHelper::GetMaxAvatarCacheAgeSec(bool isServer)
{
	return isServer ? 2592000 : 604800;
}

unsigned NetHelper::GetLoginLockSec(bool isServer)
{
	return isServer ? 30 : 1;
}

END_UPP_NAMESPACE
