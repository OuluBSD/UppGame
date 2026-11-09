#include "RemoteClient.h"
#include <CtrlLib/CtrlLib.h>

namespace Upp {

RemoteClient::RemoteClient() {
}

RemoteClient::~RemoteClient() {
	Stop();
}

void RemoteClient::Start(const String& addr) {
	Stop();
	
	int q = addr.Find(':');
	if (q >= 0) {
		host = addr.Left(q);
		port = StrInt(addr.Mid(q + 1));
	} else {
		host = addr;
		port = 8081;
	}
	
	running = true;
	client_thread.Run([=] { Run(); });
}

void RemoteClient::Stop() {
	running = false;
	socket.Close();
	if (client_thread.IsOpen())
		client_thread.Wait();
}

void RemoteClient::Run() {
	while (running) {
		if (!socket.IsOpen()) {
			if (!socket.Connect(host, port)) {
				Sleep(2000); // Wait before retry
				continue;
			}
		}
		
		String line = socket.GetLine();
		if (socket.IsError() || socket.IsEof()) {
			socket.Close();
			continue;
		}
		
		if (!line.IsEmpty()) {
			try {
				ScreenGameState state;
				LoadFromJson(state, line);
				PostCallback([=] { WhenUpdate(state); });
			} catch (...) {
				// Ignore malformed JSON
			}
		}
	}
}

}
