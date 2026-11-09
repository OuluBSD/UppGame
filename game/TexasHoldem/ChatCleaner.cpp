#include "ChatCleaner.h"

NAMESPACE_UPP

ChatCleaner::ChatCleaner(class ConfigFile* c) : config(c)
{
	messageFilter = std::make_shared<MessageFilter>(c);
}

ChatCleaner::~ChatCleaner()
{
	StopServer();
}

void ChatCleaner::RefreshConfig()
{
	messageFilter->RefreshConfig();
}

void ChatCleaner::Process()
{
	messageFilter->ProcessCleanup();
	if (serverRunning) {
		TcpSocket client;
		if (server.Accept(client)) {
			HandleClient(client);
		}
	}
}

void ChatCleaner::StartServer(int port)
{
	if (server.Listen(port, 5)) {
		serverRunning = true;
		server.Timeout(0);
	}
}

void ChatCleaner::StopServer()
{
	server.Close();
	serverRunning = false;
}

void ChatCleaner::HandleClient(TcpSocket& s)
{
	// Stub for protocol handling (Protobuf ChatCleanerMessage)
	s.Close();
}

END_UPP_NAMESPACE
