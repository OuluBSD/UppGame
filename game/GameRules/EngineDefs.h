#ifndef _CardEngine_EngineDefs_h_
#define _CardEngine_EngineDefs_h_

#include <memory>
#include <vector>
#include <Core/Core.h>

namespace Upp {

class PlayerInterface;

enum GameType {
	GAME_TYPE_NLTH,
	GAME_TYPE_PLO,
	GAME_TYPE_HEARTS,
	GAME_TYPE_PLO5
};

enum TournamentMode {
	TMODE_SINGLE_TABLE,
	TMODE_MULTI_TABLE_ROTATING,
	TMODE_WINNER_ADVANCE
};

struct HandState {
	GameType type;
	int      round;
	int      pot;
	int      dealer;
	int      active_player;
	Vector<int> board;
};

typedef Vector<int> ActionVec;
typedef std::shared_ptr<std::vector<std::shared_ptr<PlayerInterface>>> PlayerList;
typedef std::vector<std::shared_ptr<PlayerInterface>>::iterator PlayerListIterator;
typedef std::vector<std::shared_ptr<PlayerInterface>>::const_iterator PlayerListConstIterator;

}

#endif
