#ifndef _CardEngine_ScreenGameState_h_
#define _CardEngine_ScreenGameState_h_

#include <Core/Core.h>

namespace Upp {

struct ScreenGameState : public Moveable<ScreenGameState> {
	int pot = 0;
	Vector<int> stacks;
	Vector<int> community_cards;
	bool my_turn = false;
	bool found = false;
	
	ScreenGameState() {}
	ScreenGameState(const ScreenGameState& s) { *this = s; }
	
	void operator=(const ScreenGameState& s) {
		pot = s.pot;
		stacks <<= s.stacks;
		community_cards <<= s.community_cards;
		my_turn = s.my_turn;
		found = s.found;
	}
	
	void Jsonize(JsonIO& jio) {
		jio
			("pot", pot)
			("stacks", stacks)
			("community_cards", community_cards)
			("my_turn", my_turn)
			("found", found)
		;
	}
};

}

#endif
