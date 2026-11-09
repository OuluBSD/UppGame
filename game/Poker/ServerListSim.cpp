#include <Poker/ServerListSim.h>

NAMESPACE_UPP

ServerListSim& ServerListSim::GetGlobal() {
	static ServerListSim inst;
	return inst;
}

void ServerListSim::GenerateRandomList(int count) {
	Clear();
	
	const char* names[] = { "Beginner Table", "High Stakes", "Omaha Madness", "Hearts Fun", "Pro League", "Late Night Poker" };
	
	for (int i = 0; i < count; i++) {
		SimulatedServer s;
		s.id = 1000 + i;
		s.ping = 10 + rand() % 100;
		
		s.info.name = names[rand() % 6];
		s.info.mode = GAME_MODE_CREATED;
		
		// 1/3 chance for each game type
		int r = rand() % 3;
		if (r == 0) {
			s.info.data.hand_type = GAME_TYPE_NLTH;
			s.info.data.firstSmallBlind = 10;
		} else if (r == 1) {
			s.info.data.hand_type = GAME_TYPE_PLO;
			s.info.data.firstSmallBlind = 20;
		} else {
			s.info.data.hand_type = GAME_TYPE_HEARTS;
			s.info.data.firstSmallBlind = 0;
		}
		
		s.info.data.maxNumberOfPlayers = (s.info.data.hand_type == GAME_TYPE_HEARTS) ? 4 : 10;
		int p_count = 1 + rand() % s.info.data.maxNumberOfPlayers;
		for (int j = 0; j < p_count; j++) s.info.players.push_back(2000 + j);
		
		games.push_back(s);
	}
}

END_UPP_NAMESPACE
