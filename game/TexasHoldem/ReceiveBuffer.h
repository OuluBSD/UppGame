#ifndef _CardEngine_ReceiveBuffer_h_
#define _CardEngine_ReceiveBuffer_h_

#include <Core/Core.h>

NAMESPACE_UPP

class SessionData;

class ReceiveBuffer {
public:
	virtual ~ReceiveBuffer() {}

	void StartAsyncRead(std::shared_ptr<SessionData> session) {}
	void HandleRead(std::shared_ptr<SessionData> session, int errorCode, size_t bytesRead) {}
	void HandleMessage(std::shared_ptr<SessionData> session, const String &msg) {}
};

END_UPP_NAMESPACE

#endif