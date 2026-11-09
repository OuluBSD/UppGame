#ifndef _CardEngine_ServerData_h_
#define _CardEngine_ServerData_h_

#include <Core/Core.h>

NAMESPACE_UPP

struct ServerInfo {
	ServerInfo() : id(0), supportsSctp(false), useTLS(true), port(0) {}
	unsigned id;
	String name;
	String sponsor;
	String country;
	String ipv4addr;
	String ipv6addr;
	bool supportsSctp;
	bool useTLS;
	int port;
	String avatarServerAddr;
};

struct ServerStats {
	ServerStats()
		: numberOfPlayersOnServer(0), numberOfGamesOpen(0), totalPlayersEverLoggedIn(0),
		  totalGamesEverCreated(0), maxGamesOpen(0), maxPlayersLoggedIn(0) {}
	unsigned numberOfPlayersOnServer;
	unsigned numberOfGamesOpen;
	unsigned totalPlayersEverLoggedIn;
	unsigned totalGamesEverCreated;
	unsigned maxGamesOpen;
	unsigned maxPlayersLoggedIn;
};

END_UPP_NAMESPACE

#endif
