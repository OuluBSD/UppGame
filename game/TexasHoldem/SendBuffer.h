#ifndef _CardEngine_SendBuffer_h_
#define _CardEngine_SendBuffer_h_

#include <Core/Core.h>

NAMESPACE_UPP

class SessionData;
class NetPacket;

class SendBuffer {
public:
	virtual ~SendBuffer() {}

	void SetCloseAfterSend() {}
	void AsyncSendNextPacket(std::shared_ptr<SessionData> session) {}
	void InternalStorePacket(std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet) {}
	void HandleWrite(std::shared_ptr<SessionData> session, int errorCode) {}
};

END_UPP_NAMESPACE

#endif