#ifndef _GameCommon_ServerListSim_h_
#define _GameCommon_ServerListSim_h_

#include <GameRules/EngineDefs.h>
#include <GameRules/GameData.h>

NAMESPACE_UPP

struct SimulatedServer {
	unsigned id;
	GameInfo info;
	int      ping;
};

class ServerListSim {
	std::vector<SimulatedServer> games;
	
public:
	void GenerateRandomList(int count);
	void Clear() { games.clear(); }
	
	int GetCount() const { return (int)games.size(); }
	const SimulatedServer& GetGame(int i) const { return games[i]; }
	
	static ServerListSim& GetGlobal();
};

END_UPP_NAMESPACE

#endif
