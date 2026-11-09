#ifndef _game_CardGame_HeartsAI_h_
#define _game_CardGame_HeartsAI_h_

class HeartsAI {
public:
	static Vector<Card> ChoosePassCards(const Vector<Card>& hand);
	static Card ChooseCard(int player_index, const Vector<Card>& hand, const Vector<TrickItem>& trick, const String& leading_suit, bool hearts_broken);
};

#endif
