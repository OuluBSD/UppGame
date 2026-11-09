#ifndef _game_CardGame_Card_h_
#define _game_CardGame_Card_h_

class Card : public Moveable<Card> {
public:
	String suit;
	String rank;
	String id;

	Card();
	Card(const String& suit, const String& rank);

	int GetPoints() const;
	String ToString() const;

	bool operator==(const Card& other) const { return id == other.id; }
	bool operator!=(const Card& other) const { return id != other.id; }
	
	// Support U++ serialization if needed
	void Jsonize(JsonIO& json);
};

int RankIndex(const String& rank);

#endif
