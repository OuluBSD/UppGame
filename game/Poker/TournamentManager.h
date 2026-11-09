#ifndef _GameCommon_TournamentManager_h_
#define _GameCommon_TournamentManager_h_

#include <GameRules/Game.h>

NAMESPACE_UPP

struct TournamentTable {
	int id;
	std::shared_ptr<Game> game;
	Vector<unsigned> player_ids;
};

class TournamentManager {
	TournamentMode mode;
	Array<TournamentTable> tables;
	std::vector<std::shared_ptr<PlayerData>> players;
	GuiInterface* m_gui = nullptr;
	
public:
	TournamentManager(TournamentMode m) : mode(m) {}
	
	void AddPlayer(std::shared_ptr<PlayerData> pd) { players.push_back(pd); }
	void Start(GuiInterface* gui = nullptr);
	void Update(); // Balance tables, handle eliminations
	
	void RotateTables(); // TMODE_MULTI_TABLE_ROTATING
	void AdvanceWinners(); // TMODE_WINNER_ADVANCE
	
	int GetTableCount() const { return tables.GetCount(); }
	Game* GetTableGame(int i) { return tables[i].game.get(); }
	
	int GetPlayerCount() const { return (int)players.size(); }
	PlayerData* GetPlayerData(int i) { return players[i].get(); }
};

END_UPP_NAMESPACE

#endif
