#include "ClientState.h"
#include "ClientThread.h"
#include "NetPacket.h"

NAMESPACE_UPP

// ClientStateInit implementation
void ClientStateInit::Enter(std::shared_ptr<ClientThread> thread)
{
}

void ClientStateInit::Exit(std::shared_ptr<ClientThread> thread)
{
}

void ClientStateInit::HandlePacket(std::shared_ptr<ClientThread> thread, std::shared_ptr<NetPacket> packet)
{
	int type = packet->GetType();
	if (type == NetPacket::Type_InitAckMessage) {
		auto* msg = dynamic_cast<InitAckMsg*>(packet->GetMessage());
		if (msg) {
			thread->SetGuiPlayerId(msg->yourPlayerId);
		}
	}
}

// Stub for Instance()
ClientStateInit &ClientStateInit::Instance()
{
	static ClientStateInit instance;
	return instance;
}

END_UPP_NAMESPACE
