#ifndef _CardEngine_SenderHelper_h_
#define _CardEngine_SenderHelper_h_

#include <memory>
#include "NetPacket.h"

NAMESPACE_UPP

class SessionData;

class SenderHelper
{
public:
	SenderHelper() {}
	~SenderHelper() {}

	void Send(std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet) {}
	void Send(std::shared_ptr<SessionData> session, const NetPacketList &packetList) {}

	void SetCloseAfterSend(std::shared_ptr<SessionData> session) {}
};

END_UPP_NAMESPACE

#endif
