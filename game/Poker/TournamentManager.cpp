#include <Poker/TournamentManager.h>
#include <Poker/LocalEngineFactory.h>
#include <GameRules/EngineLog.h>
#include <GameRules/PlayerInterface.h>
#include <GameRules/LocalPlayer.h>
#include <EditorCommon/ConfigFile.h>
#include <iostream>

NAMESPACE_UPP

void TournamentManager::Start(GuiInterface* gui) {
	m_gui = gui;
	tables.Clear();
	
	int players_per_table = 10;
	int num_tables = ((int)players.size() + players_per_table - 1) / players_per_table;
	
	auto factory = std::make_shared<LocalEngineFactory>();
	static class ConfigFile cfg(nullptr, false);
	cfg.writeConfigInt("AI_BACKEND", 0);
	cfg.writeConfigInt("AI_DISABLE_ALLIN", 0);
	
	for (int i = 0; i < num_tables; i++) {
		TournamentTable& tt = tables.Add();
		tt.id = i;
		
		PlayerDataList table_pd;
		tt.player_ids.Clear();
		for (int j = 0; j < players_per_table; j++) {
			int idx = i * players_per_table + j;
			if (idx >= (int)players.size()) break;
			table_pd.push_back(players[idx]);
			tt.player_ids.Add(players[idx]->GetUniqueId());
		}
		
		GameData gData;
		gData.startMoney = 1000;
		gData.firstSmallBlind = 500;
		
		StartData sData;
		sData.numberOfPlayers = (int)table_pd.size();
		sData.startDealerPlayerId = table_pd[0]->GetUniqueId();
		
		tt.game = std::make_shared<Game>(m_gui, factory, table_pd, gData, sData, i, nullptr, &cfg);
	}
}

void TournamentManager::Update() {
	// Synchronize player cash back to PlayerData
	for (auto& tt : tables) {
		for (unsigned id : tt.player_ids) {
			auto p = tt.game->getPlayerByUniqueId(id);
			if (p) {
				for (auto& pd : players) {
					if (pd->GetUniqueId() == id) {
						pd->SetStartCash(p->getMyCash());
						break;
					}
				}
			}
		}
	}

	// Eliminate players with cash <= 0
	bool changed = false;
	for (auto it = players.begin(); it != players.end(); ) {
		if ((*it)->GetStartCash() <= 0) {
			it = players.erase(it);
			changed = true;
		} else {
			++it;
		}
	}
	
	if (changed && !players.empty()) {
		std::cout << "DEBUG: Players eliminated. Re-seating remaining " << players.size() << " players." << std::endl;
		Start(m_gui);
	}
}

void TournamentManager::RotateTables() {
	if (mode != TMODE_MULTI_TABLE_ROTATING || players.size() < 2) return;
	
	std::rotate(players.begin(), players.begin() + 1, players.end());
	Start(m_gui);
}

void TournamentManager::AdvanceWinners() {
	if (mode != TMODE_WINNER_ADVANCE || tables.IsEmpty()) return;
	
	bool all_done = true;
	for (auto& tt : tables) if (!tt.game->isGameOver()) { all_done = false; break; }
	if (!all_done) return;
	
	std::vector<std::shared_ptr<PlayerData>> winners;
	for (auto& tt : tables) {
		for (unsigned id : tt.player_ids) {
			auto p = tt.game->getPlayerByUniqueId(id);
			if (p && p->getMyCash() > 0) {
				auto pd = std::make_shared<PlayerData>(p->getMyUniqueID(), p->getMyID(), p->getMyType(), PLAYER_RIGHTS_NORMAL, false);
				pd->SetName(p->getMyName());
				pd->SetStartCash(p->getMyCash());
				winners.push_back(pd);
			}
		}
	}
	
	players = winners;
	Start(m_gui);
}

END_UPP_NAMESPACE
