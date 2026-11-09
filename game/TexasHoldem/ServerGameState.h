#ifndef _CardEngine_ServerGameState_h_
#define _CardEngine_ServerGameState_h_

#include <Core/Core.h>
#include <memory>
#include "SessionData.h"

NAMESPACE_UPP

class ServerGame;
class NetPacket;
class Game;
class PlayerData;

class ServerGameState
{
public:
	virtual ~ServerGameState() {}

	virtual void Enter(std::shared_ptr<ServerGame> server) = 0;
	virtual void Exit(std::shared_ptr<ServerGame> server) = 0;

	virtual void NotifyGameAdminChanged(std::shared_ptr<ServerGame> server) = 0;
	virtual void NotifySessionRemoved(std::shared_ptr<ServerGame> server) = 0;

	virtual void HandleNewPlayer(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session) = 0;
	virtual void HandleNewSpectator(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session) = 0;

	virtual void ProcessPacket(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet) = 0;
};

class AbstractServerGameStateReceiving : public ServerGameState
{
public:
	virtual ~AbstractServerGameStateReceiving() {}

	virtual void ProcessPacket(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet) override;

protected:
	AbstractServerGameStateReceiving() {}

	std::shared_ptr<NetPacket> CreateNetPacketPlayerJoined(unsigned gameId, const PlayerData &playerData);
	std::shared_ptr<NetPacket> CreateNetPacketSpectatorJoined(unsigned gameId, const PlayerData &playerData);
	std::shared_ptr<NetPacket> CreateNetPacketJoinGameAck(const ServerGame &server, const PlayerData &playerData, bool spectateOnly);
	std::shared_ptr<NetPacket> CreateNetPacketHandStart(const ServerGame &server);

	void AcceptNewSession(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session, bool spectateOnly);

	virtual void InternalProcessPacket(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet) = 0;
};

class ServerGameStateInit : public AbstractServerGameStateReceiving
{
public:
	static ServerGameStateInit& Instance();
	virtual ~ServerGameStateInit() {}

	virtual void Enter(std::shared_ptr<ServerGame> server) override;
	virtual void Exit(std::shared_ptr<ServerGame> server) override;

	virtual void NotifyGameAdminChanged(std::shared_ptr<ServerGame> server) override;
	virtual void NotifySessionRemoved(std::shared_ptr<ServerGame> server) override;

	virtual void HandleNewPlayer(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session) override;
	virtual void HandleNewSpectator(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session) override;

protected:
	ServerGameStateInit() {}
	virtual void InternalProcessPacket(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet) override;
};

class ServerGameStateHand : public ServerGameState {
public:
	static ServerGameStateHand& Instance();
	virtual ~ServerGameStateHand() {}

	virtual void Enter(std::shared_ptr<ServerGame> server) override;
	virtual void Exit(std::shared_ptr<ServerGame> server) override;

	virtual void NotifyGameAdminChanged(std::shared_ptr<ServerGame> server) override;
	virtual void NotifySessionRemoved(std::shared_ptr<ServerGame> server) override;

	virtual void HandleNewPlayer(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session) override;
	virtual void HandleNewSpectator(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session) override;

	virtual void ProcessPacket(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet) override;

protected:
	ServerGameStateHand() {}
};

class ServerGameStateFinal : public ServerGameState {
public:
	static ServerGameStateFinal& Instance();
	virtual ~ServerGameStateFinal() {}

	virtual void Enter(std::shared_ptr<ServerGame> server) override;
	virtual void Exit(std::shared_ptr<ServerGame> server) override;

	virtual void NotifyGameAdminChanged(std::shared_ptr<ServerGame> server) override;
	virtual void NotifySessionRemoved(std::shared_ptr<ServerGame> server) override;

	virtual void HandleNewPlayer(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session) override;
	virtual void HandleNewSpectator(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session) override;

	virtual void ProcessPacket(std::shared_ptr<ServerGame> server, std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet) override;

protected:
	ServerGameStateFinal() {}
};

END_UPP_NAMESPACE

#endif