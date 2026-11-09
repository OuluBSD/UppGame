#include "ChatCleanerManager.h"

NAMESPACE_UPP

ChatCleanerManager::ChatCleanerManager(ChatCleanerCallback &cb)
	: m_callback(cb)
{
}

ChatCleanerManager::~ChatCleanerManager()
{
	SignalTermination();
}

void ChatCleanerManager::Init(const String &serverAddr, int port, bool ipv6,
						 const String &clientSecret, const String &serverSecret)
{
	m_serverAddr = serverAddr;
	m_serverPort = port;
	m_useIpv6 = ipv6;
	m_clientSecret = clientSecret;
	m_serverSecret = serverSecret;
	ReInit();
}

void ChatCleanerManager::ReInit()
{
	// TODO: Trigger reconnection in Main thread
}

void ChatCleanerManager::HandleLobbyChatText(unsigned playerId, const String &name, const String &text)
{
	HandleGameChatText(0, playerId, name, text);
}

void ChatCleanerManager::HandleGameChatText(unsigned gameId, unsigned playerId, const String &name, const String &text)
{
	if (m_connected) {
		// TODO: Send message
	}
}

void ChatCleanerManager::Main()
{
	// TODO: Implementation of network loop
}

void ChatCleanerManager::SendMessageToServer(const String &msgType, const ValueMap &data)
{
	// TODO: Implementation
}

unsigned ChatCleanerManager::GetNextRequestId()
{
	m_curRequestId++;
	if (m_curRequestId == 0) m_curRequestId++;
	return m_curRequestId;
}

END_UPP_NAMESPACE
