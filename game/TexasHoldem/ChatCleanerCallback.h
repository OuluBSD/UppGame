#ifndef _CardEngine_ChatCleanerCallback_h_
#define _CardEngine_ChatCleanerCallback_h_

#include <Core/Core.h>

NAMESPACE_UPP

class ChatCleanerCallback
{
public:
	virtual ~ChatCleanerCallback() {}

	virtual void SignalChatBotMessage(const String &msg) = 0;
	virtual void SignalChatBotMessage(unsigned gameId, const String &msg) = 0;
	virtual void SignalKickPlayer(unsigned playerId) = 0;
	virtual void SignalBanPlayer(unsigned playerId) = 0;
	virtual void SignalMutePlayer(unsigned playerId) = 0;
};

END_UPP_NAMESPACE

#endif
