#ifndef _CardEngine_GameData_h_
#define _CardEngine_GameData_h_

#include <Core/Core.h>
#include <vector>
#include <GameRules/GameDefs.h>
#include <GameRules/EngineDefs.h>

NAMESPACE_UPP

typedef Vector<unsigned> PlayerIdList;

enum GameMode {
	GAME_MODE_CREATED = 1,
	GAME_MODE_STARTED,
	GAME_MODE_CLOSED
};

enum NetworkGameType {
	NET_GAME_TYPE_NORMAL = 1,
	NET_GAME_TYPE_REGISTERED_ONLY,
	NET_GAME_TYPE_INVITE_ONLY,
	NET_GAME_TYPE_RANKING
};

enum RaiseIntervalMode {
	RAISE_ON_HANDNUMBER = 1,
	RAISE_ON_MINUTES
};

enum RaiseMode {
	DOUBLE_BLINDS = 1,
	MANUAL_BLINDS_ORDER
};

enum AfterManualBlindsMode {
	AFTERMB_DOUBLE_BLINDS = 1,
	AFTERMB_RAISE_ABOUT,
	AFTERMB_STAY_AT_LAST_BLIND
};

struct GameData {
	GameData() : gameType(NET_GAME_TYPE_NORMAL), allowSpectators(true),
		maxNumberOfPlayers(0), startMoney(0), firstSmallBlind(0),
		raiseIntervalMode(RAISE_ON_HANDNUMBER), raiseSmallBlindEveryHandsValue(8),
		raiseSmallBlindEveryMinutesValue(1), raiseMode(DOUBLE_BLINDS),
		afterManualBlindsMode(AFTERMB_DOUBLE_BLINDS), afterMBAlwaysRaiseValue(0),
		guiSpeed(4), delayBetweenHandsSec(6), playerActionTimeoutSec(20),
		hand_type(GAME_TYPE_NLTH) {}
	
	NetworkGameType gameType;
	bool allowSpectators;
	int maxNumberOfPlayers;
	int startMoney;
	int firstSmallBlind;
	RaiseIntervalMode raiseIntervalMode;
	int raiseSmallBlindEveryHandsValue;
	int raiseSmallBlindEveryMinutesValue;
	RaiseMode raiseMode;
	std::vector<int> manualBlindsList; // std::vector is copyable
	AfterManualBlindsMode afterManualBlindsMode;
	int afterMBAlwaysRaiseValue;
	int guiSpeed;
	int delayBetweenHandsSec;
	int playerActionTimeoutSec;
	GameType hand_type;

	void Serialize(Stream& s) {
		int gt = (int)gameType;
		s % gt; gameType = (NetworkGameType)gt;
		s % allowSpectators % maxNumberOfPlayers % startMoney % firstSmallBlind;
		int rim = (int)raiseIntervalMode;
		s % rim; raiseIntervalMode = (RaiseIntervalMode)rim;
		s % raiseSmallBlindEveryHandsValue % raiseSmallBlindEveryMinutesValue;
		int rm = (int)raiseMode;
		s % rm; raiseMode = (RaiseMode)rm;
		
		int count = (int)manualBlindsList.size();
		s % count;
		if (s.IsLoading()) {
			manualBlindsList.resize(count);
			for (int i = 0; i < count; ++i) s % manualBlindsList[i];
		} else {
			for (int i = 0; i < count; ++i) s % manualBlindsList[i];
		}

		int ammbm = (int)afterManualBlindsMode;
		s % ammbm; afterManualBlindsMode = (AfterManualBlindsMode)ammbm;
		s % afterMBAlwaysRaiseValue % guiSpeed % delayBetweenHandsSec % playerActionTimeoutSec;
		
		int ht = (int)hand_type;
		s % ht; hand_type = (GameType)ht;
	}
};

struct GameInfo {
	GameInfo() : mode(GAME_MODE_CREATED), adminPlayerId(0), isPasswordProtected(false) {}
	
	String name;
	GameData data;
	GameMode mode;
	unsigned adminPlayerId;
	std::vector<unsigned> players; // std::vector is copyable
	std::vector<unsigned> spectators;
	std::vector<unsigned> spectatorsDuringGame;
	bool isPasswordProtected;

	void Serialize(Stream& s) {
		s % name % data;
		int m = (int)mode;
		s % m; mode = (GameMode)m;
		s % adminPlayerId;
		
		auto serialize_vec = [&](std::vector<unsigned>& v) {
			int count = (int)v.size();
			s % count;
			if (s.IsLoading()) {
				v.resize(count);
				for (int i = 0; i < count; ++i) s % v[i];
			} else {
				for (int i = 0; i < count; ++i) s % v[i];
			}
		};
		serialize_vec(players);
		serialize_vec(spectators);
		serialize_vec(spectatorsDuringGame);
		
		s % isPasswordProtected;
	}
};

struct StartData {
	StartData() : startDealerPlayerId(0), numberOfPlayers(0) {}
	unsigned startDealerPlayerId;
	int numberOfPlayers;

	void Serialize(Stream& s) {
		s % startDealerPlayerId % numberOfPlayers;
	}
};

struct VoteKickData {
	VoteKickData()
		: petitionId(0), kickPlayerId(0), numVotesToKick(0),
		  numVotesInFavourOfKicking(0), numVotesAgainstKicking(0), timeLimitSec(0)
	{
		voteTimer.Reset();
	}
	
	unsigned petitionId;
	unsigned kickPlayerId;
	int numVotesToKick;
	int numVotesInFavourOfKicking;
	int numVotesAgainstKicking;
	int timeLimitSec;
	TimeStop voteTimer;
	std::vector<unsigned> votedPlayerIds;

	void Serialize(Stream& s) {
		s % petitionId % kickPlayerId % numVotesToKick % numVotesInFavourOfKicking % numVotesAgainstKicking % timeLimitSec;
		int count = (int)votedPlayerIds.size();
		s % count;
		if (s.IsLoading()) {
			votedPlayerIds.resize(count);
			for (int i = 0; i < count; ++i) s % votedPlayerIds[i];
		} else {
			for (int i = 0; i < count; ++i) s % votedPlayerIds[i];
		}
	}
};

END_UPP_NAMESPACE

#endif
