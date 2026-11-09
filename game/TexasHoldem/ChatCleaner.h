#ifndef _CardEngine_ChatCleaner_h_
#define _CardEngine_ChatCleaner_h_

#include <memory>
#include "ChatFilters.h"
#include <EditorCommon/ConfigFile.h>

NAMESPACE_UPP

class ChatCleaner
{
public:
	ChatCleaner(class ConfigFile* c);
	~ChatCleaner();

	void RefreshConfig();
	void Process();

	// If we want to support the network protocol:
	void StartServer(int port);
	void StopServer();

private:
	class ConfigFile* config;
	std::shared_ptr<MessageFilter> messageFilter;
	
	TcpSocket server;
	bool serverRunning = false;
	
	void HandleClient(TcpSocket& s);
};

END_UPP_NAMESPACE

#endif
