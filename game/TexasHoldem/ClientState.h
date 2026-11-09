#ifndef _CardEngine_ClientState_h_
#define _CardEngine_ClientState_h_

#include <Core/Core.h>
#include <memory>

#define CLIENT_INITIAL_STATE ClientStateInit
#define CLIENT_FINAL_STATE ClientStateFinal

NAMESPACE_UPP

class ClientThread;
class NetPacket;

class ClientState
{
public:
	virtual ~ClientState() {}

	virtual void Enter(std::shared_ptr<ClientThread> client) = 0;
	virtual void Exit(std::shared_ptr<ClientThread> client) = 0;

	virtual void HandlePacket(std::shared_ptr<ClientThread> client, std::shared_ptr<NetPacket> tmpPacket) = 0;
};

class ClientStateInit : public ClientState
{
public:
	static ClientStateInit &Instance();
	virtual ~ClientStateInit() {}

	virtual void Enter(std::shared_ptr<ClientThread> client) override;
	virtual void Exit(std::shared_ptr<ClientThread> client) override;
	virtual void HandlePacket(std::shared_ptr<ClientThread> client, std::shared_ptr<NetPacket> tmpPacket) override;

protected:
	ClientStateInit() {}
};

class AbstractClientStateReceiving : public ClientState
{
public:
	virtual ~AbstractClientStateReceiving() {}
	virtual void HandlePacket(std::shared_ptr<ClientThread> client, std::shared_ptr<NetPacket> tmpPacket) override;

protected:
	AbstractClientStateReceiving() {}
	virtual void InternalHandlePacket(std::shared_ptr<ClientThread> client, std::shared_ptr<NetPacket> tmpPacket) = 0;
};

class ClientStateRunHand : public AbstractClientStateReceiving
{
public:
	static ClientStateRunHand &Instance();
	virtual ~ClientStateRunHand() {}

	virtual void Enter(std::shared_ptr<ClientThread> client) override;
	virtual void Exit(std::shared_ptr<ClientThread> client) override;

protected:
	ClientStateRunHand() {}
	virtual void InternalHandlePacket(std::shared_ptr<ClientThread> client, std::shared_ptr<NetPacket> tmpPacket) override;
};

class ClientStateFinal : public ClientState
{
public:
	static ClientStateFinal &Instance();
	virtual ~ClientStateFinal() {}

	virtual void Enter(std::shared_ptr<ClientThread> client) override {}
	virtual void Exit(std::shared_ptr<ClientThread> client) override {}
	virtual void HandlePacket(std::shared_ptr<ClientThread> client, std::shared_ptr<NetPacket> tmpPacket) override {}

protected:
	ClientStateFinal() {}
};

END_UPP_NAMESPACE

#endif
