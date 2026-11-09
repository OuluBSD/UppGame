#ifndef _CardEngine_NetHelper_h_
#define _CardEngine_NetHelper_h_

#include <Core/Core.h>

#define SERVER_COMPUTER_PLAYER_NAME			"Computer"
#define SERVER_GUEST_PLAYER_NAME			"Guest"

NAMESPACE_UPP

class NetHelper
{
public:
	static unsigned GetMaxNumberOfAvatarFiles(bool isServer = true);
	static unsigned GetMaxAvatarCacheAgeSec(bool isServer = true);
	static unsigned GetLoginLockSec(bool isServer = true);
	
	// Compatibility methods
	static unsigned GetMaxNumberOfAvatarFiles() { return GetMaxNumberOfAvatarFiles(true); }
	static unsigned GetMaxAvatarCacheAgeSec() { return GetMaxAvatarCacheAgeSec(true); }
	static unsigned GetLoginLockSec() { return GetLoginLockSec(true); }
};

END_UPP_NAMESPACE

#endif