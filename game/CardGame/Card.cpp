#include "CardGame.h"

NAMESPACE_UPP

Card::Card() {}

Card::Card(const String& suit, const String& rank) : suit(suit), rank(rank) {
	id = suit + "_" + rank;
}

int Card::GetPoints() const {
	if (suit == "hearts")
		return 1;
	if (suit == "spades" && rank == "queen")
		return 13;
	return 0;
}

String Card::ToString() const {
	return id;
}

void Card::Jsonize(JsonIO& json) {
	json("suit", suit)("rank", rank)("id", id);
}

int RankIndex(const String& rank) {
	static const char* RANKS[] = { "2", "3", "4", "5", "6", "7", "8", "9", "10", "jack", "queen", "king", "ace" };
	for (int i = 0; i < 13; i++) {
		if (RANKS[i] == rank)
			return i;
	}
	return -1;
}

END_UPP_NAMESPACE
