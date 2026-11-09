#ifndef _CardEngine_CardsValue_h_
#define _CardEngine_CardsValue_h_

#include <GameRules/EngineDefs.h>
#include <GameRules/GameDefs.h>

NAMESPACE_UPP

class CardsValue
{
public:
	static int holeCardsClass(int one, int two);
	static int holeCardsToIntCode(int* cards);
	static int cardsValue(int* cards, int* position);
	static Vector<Vector<int>> calcCardsChance(TexasRound beRoID, int* playerCards, int* boardCards);
	static String determineHandName(int myCardsValueInt, PlayerList activePlayerList);
	static Vector<String> translateCardsValueCode(int cardsValueCode);
};

END_UPP_NAMESPACE

#endif