#ifndef _CardEngine_ChatCleanerManager_h_
#define _CardEngine_ChatCleanerManager_h_

#include <Core/Core.h>
#include "Thread.h"
#include "ChatCleanerCallback.h"

#define CLEANER_NET_HEADER_SIZE		4
#define MAX_CLEANER_PACKET_SIZE		512
#define CLEANER_PROTOCOL_VERSION	2

#include <memory>

NAMESPACE_UPP

class ChatCleanerManager : public ThreadWrapper, public std::enable_shared_from_this<ChatCleanerManager>
{
public:
	ChatCleanerManager(ChatCleanerCallback &cb);
	virtual ~ChatCleanerManager();

	void Init(const String &serverAddr, int port, bool ipv6,
			  const String &clientSecret, const String &serverSecret);
	void ReInit();
	void HandleLobbyChatText(unsigned playerId, const String &name, const String &text);
	void HandleGameChatText(unsigned gameId, unsigned playerId, const String &name, const String &text);

protected:
	virtual void Main() override;
	void SendMessageToServer(const String &msgType, const ValueMap &data);
	unsigned GetNextRequestId();

private:
	ChatCleanerCallback &m_callback;
	TcpSocket m_socket;
	bool m_connected = false;
	unsigned m_curRequestId = 0;
	String m_serverAddr;
	int m_serverPort = 0;
	bool m_useIpv6 = false;
	String m_clientSecret;
	String m_serverSecret;
	
	Mutex m_sendMutex;
};

END_UPP_NAMESPACE

#endif
