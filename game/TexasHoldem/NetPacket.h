#ifndef _CardEngine_NetPacket_h_
#define _CardEngine_NetPacket_h_

#include <Core/Core.h>
#include <memory>
#include <GameRules/GameData.h>

NAMESPACE_UPP

#define NET_VERSION_MAJOR			5
#define NET_VERSION_MINOR			1

#define NET_HEADER_SIZE				4

#define MAX_FILE_DATA_SIZE			256
#define MAX_PACKET_SIZE				384
#define MAX_CHAT_TEXT_SIZE			128

enum NetGameMode {
	netGameCreated = 1,
	netGameStarted = 2,
	netGameClosed = 3
};

enum NetGameState {
	netStatePreflop = 0,
	netStateFlop = 1,
	netStateTurn = 2,
	netStateRiver = 3,
	netStatePreflopSmallBlind = 4,
	netStatePreflopBigBlind = 5
};

enum NetPlayerAction {
	netActionNone = 0,
	netActionFold = 1,
	netActionCheck = 2,
	netActionCall = 3,
	netActionBet = 4,
	netActionRaise = 5,
	netActionAllIn = 6
};

enum NetPlayerState {
	netPlayerStateNormal = 0,
	netPlayerStateSessionInactive = 1,
	netPlayerStateNoMoney = 2
};

enum NetPlayerInfoRights {
	netPlayerRightsGuest = 1,
	netPlayerRightsNormal = 2,
	netPlayerRightsAdmin = 3
};

enum NetAvatarType {
	netAvatarImagePng = 1,
	netAvatarImageJpg = 2,
	netAvatarImageGif = 3
};

struct NetGameInfoMsg : Moveable<NetGameInfoMsg> {
	String gameName;
	int netGameType;
	uint32 maxNumPlayers;
	int raiseIntervalMode;
	uint32 raiseEveryHands;
	uint32 raiseEveryMinutes;
	int endRaiseMode;
	uint32 endRaiseSmallBlindValue;
	uint32 proposedGuiSpeed;
	uint32 delayBetweenHands;
	uint32 playerActionTimeout;
	uint32 firstSmallBlind;
	uint32 startMoney;
	Vector<uint32> manualBlinds;
	bool allowSpectators;

	void Serialize(Stream& s) {
		s % gameName % netGameType % maxNumPlayers % raiseIntervalMode % raiseEveryHands % raiseEveryMinutes 
		  % endRaiseMode % endRaiseSmallBlindValue % proposedGuiSpeed % delayBetweenHands % playerActionTimeout 
		  % firstSmallBlind % startMoney % manualBlinds % allowSpectators;
	}
};

struct NetPlayerResultMsg : Moveable<NetPlayerResultMsg> {
	uint32 playerId;
	uint32 resultCard1;
	uint32 resultCard2;
	Vector<uint32> bestHandPosition;
	uint32 moneyWon;
	uint32 playerMoney;
	uint32 cardsValue;

	void Serialize(Stream& s) {
		s % playerId % resultCard1 % resultCard2 % bestHandPosition % moneyWon % playerMoney % cardsValue;
	}
};

class NetMessage {
public:
	virtual ~NetMessage() {}
	virtual void Serialize(Stream& s) = 0;
	virtual int GetType() const = 0;
};

#define DECLARE_NET_MESSAGE(name, type_id) \
struct name##Msg : public NetMessage, public Moveable<name##Msg> { \
	virtual void Serialize(Stream& s) override; \
	virtual int GetType() const override { return type_id; }

// --- Protocol Messages (IDs aligned with PokerTH protobuf field numbers) ---

// 2: Announce
DECLARE_NET_MESSAGE(Announce, 2)
	struct Version : Moveable<Version> {
		uint32 major;
		uint32 minor;
		void Serialize(Stream& s) { s % major % minor; }
	};
	Version protocolVersion;
	Version latestGameVersion;
	uint32 latestBetaRevision;
	int serverType;
	uint32 numPlayersOnServer;
};

// 3: Init
DECLARE_NET_MESSAGE(Init, 3)
	AnnounceMsg::Version requestedVersion;
	uint32 buildId;
	String myLastSessionId;
	String authServerPassword;
	int login;
	String nickName;
	String clientUserData;
	String avatarHash;
};

// 4: AuthServerChallenge
DECLARE_NET_MESSAGE(AuthServerChallenge, 4)
	String challenge;
};

// 5: AuthClientResponse
DECLARE_NET_MESSAGE(AuthClientResponse, 5)
	String response;
};

// 6: AuthServerVerification
DECLARE_NET_MESSAGE(AuthServerVerification, 6)
	bool verified;
};

// 7: InitAck
DECLARE_NET_MESSAGE(InitAck, 7)
	String yourSessionId;
	uint32 yourPlayerId;
	String yourAvatarHash;
	uint32 rejoinGameId;
};

// 8: AvatarRequest
DECLARE_NET_MESSAGE(AvatarRequest, 8)
	uint32 playerId;
	String avatarHash;
};

// 9: AvatarHeader
DECLARE_NET_MESSAGE(AvatarHeader, 9)
	uint32 playerId;
	String avatarHash;
	int avatarType;
	uint32 totalSize;
};

// 10: AvatarData
DECLARE_NET_MESSAGE(AvatarData, 10)
	uint32 playerId;
	String data;
};

// 11: AvatarEnd
DECLARE_NET_MESSAGE(AvatarEnd, 11)
	uint32 playerId;
};

// 13: PlayerList
DECLARE_NET_MESSAGE(PlayerList, 13)
	struct PlayerEntry : Moveable<PlayerEntry> {
		uint32 playerId;
		String name;
		int rights;
		void Serialize(Stream& s) { s % playerId % name % rights; }
	};
	Vector<PlayerEntry> players;
};

// 14: GameListNew
DECLARE_NET_MESSAGE(GameListNew, 14)
	struct GameEntry : Moveable<GameEntry> {
		uint32 gameId;
		String name;
		uint32 players;
		uint32 maxPlayers;
		bool isPasswordProtected;
		void Serialize(Stream& s) { s % gameId % name % players % maxPlayers % isPasswordProtected; }
	};
	Vector<GameEntry> games;
};

// 15: GameListUpdate
DECLARE_NET_MESSAGE(GameListUpdate, 15)
	uint32 gameId;
	int players;
	int maxPlayers;
	int state; 
};

// 16: GameListPlayerJoined
DECLARE_NET_MESSAGE(GameListPlayerJoined, 16)
	uint32 gameId;
	uint32 playerId;
};

// 17: GameListPlayerLeft
DECLARE_NET_MESSAGE(GameListPlayerLeft, 17)
	uint32 gameId;
	uint32 playerId;
};

// 19: PlayerInfoRequest
DECLARE_NET_MESSAGE(PlayerInfoRequest, 19)
	uint32 playerId;
};

// 20: PlayerInfoReply
DECLARE_NET_MESSAGE(PlayerInfoReply, 20)
	uint32 playerId;
	String name;
	int rights;
	String avatarHash;
	uint32 gamesPlayed;
	uint32 gamesWon;
};

// 22: JoinExistingGame
DECLARE_NET_MESSAGE(JoinExistingGame, 22)
	uint32 gameId;
	String gamePassword;
	bool autoLeave;
};

// 23: JoinNewGame
DECLARE_NET_MESSAGE(JoinNewGame, 23)
	NetGameInfoMsg gameInfo;
	String gamePassword;
	bool autoLeave;
};

// 25: JoinGameAck
DECLARE_NET_MESSAGE(JoinGameAck, 25)
	uint32 gameId;
	NetGameInfoMsg gameInfo;
	bool isAdmin;
};

// 26: JoinGameFailed
DECLARE_NET_MESSAGE(JoinGameFailed, 26)
	uint32 gameId;
	int reason;
};

// 27: GamePlayerJoined
DECLARE_NET_MESSAGE(GamePlayerJoined, 27)
	uint32 gameId;
	uint32 playerId;
	String playerName;
};

// 28: GamePlayerLeft
DECLARE_NET_MESSAGE(GamePlayerLeft, 28)
	uint32 gameId;
	uint32 playerId;
};

// 31: KickPlayerRequest
DECLARE_NET_MESSAGE(KickPlayerRequest, 31)
	uint32 gameId;
	uint32 playerId;
};

// 32: LeaveGameRequest
DECLARE_NET_MESSAGE(LeaveGameRequest, 32)
	uint32 gameId;
};

// 39: GameStartInitial
DECLARE_NET_MESSAGE(GameStartInitial, 39)
	uint32 gameId;
	Vector<uint32> playerIds;
	Vector<String> playerNames;
	Vector<uint32> playerMoney;
};

// 41: HandStart
DECLARE_NET_MESSAGE(HandStart, 41)
	uint32 gameId;
	struct PlainCards : Moveable<PlainCards> {
		uint32 card1, card2;
		void Serialize(Stream& s) { s % card1 % card2; }
	};
	One<PlainCards> plainCards;
	String encryptedCards;
	uint32 smallBlind;
	Vector<int> seatStates;
	uint32 dealerPlayerId;
};

// 42: PlayersTurn
DECLARE_NET_MESSAGE(PlayersTurn, 42)
	uint32 gameId;
	uint32 playerId;
	int gameState;
};

// 43: MyActionRequest
DECLARE_NET_MESSAGE(MyActionRequest, 43)
	uint32 gameId;
	uint32 handNum;
	int gameState;
	int myAction;
	uint32 myRelativeBet;
};

// 45: PlayersActionDone
DECLARE_NET_MESSAGE(PlayersActionDone, 45)
	uint32 gameId;
	uint32 playerId;
	int gameState;
	int playerAction;
	uint32 totalPlayerBet;
	uint32 playerMoney;
	uint32 highestSet;
	uint32 minimumRaise;
};

// 46: DealFlopCards
DECLARE_NET_MESSAGE(DealFlopCards, 46)
	uint32 gameId;
	uint32 card1, card2, card3;
};

// 47: DealTurnCard
DECLARE_NET_MESSAGE(DealTurnCard, 47)
	uint32 gameId;
	uint32 card;
};

// 48: DealRiverCard
DECLARE_NET_MESSAGE(DealRiverCard, 48)
	uint32 gameId;
	uint32 card;
};

// 49: AllInShowCards
DECLARE_NET_MESSAGE(AllInShowCards, 49)
	struct PlayerAllIn : Moveable<PlayerAllIn> {
		uint32 playerId;
		uint32 card1, card2;
		void Serialize(Stream& s) { s % playerId % card1 % card2; }
	};
	uint32 gameId;
	Vector<PlayerAllIn> playersAllIn;
};

// 50: EndOfHandShowCards
DECLARE_NET_MESSAGE(EndOfHandShowCards, 50)
	uint32 gameId;
	Vector<NetPlayerResultMsg> playerResults;
};

// 51: EndOfHandHideCards
DECLARE_NET_MESSAGE(EndOfHandHideCards, 51)
	uint32 gameId;
	uint32 playerId;
	uint32 moneyWon;
	uint32 playerMoney;
};

// 54: EndOfGame
DECLARE_NET_MESSAGE(EndOfGame, 54)
	uint32 gameId;
	uint32 winnerPlayerId;
};

// 56: AskKickPlayer
DECLARE_NET_MESSAGE(AskKickPlayer, 56)
	uint32 gameId;
	uint32 playerId;
};

// 59: VoteKickRequest
DECLARE_NET_MESSAGE(VoteKickRequest, 59)
	uint32 gameId;
	uint32 targetPlayerId;
	String reason;
};

// 60: VoteKickReply
DECLARE_NET_MESSAGE(VoteKickReply, 60)
	uint32 gameId;
	bool vote;
};

// 65: Chat
DECLARE_NET_MESSAGE(Chat, 65)
	uint32 gameId;
	uint32 playerId;
	int chatType;
	String chatText;
};

// 74: Error
DECLARE_NET_MESSAGE(Error, 74)
	int errorReason;
};

class NetPacket : public Moveable<NetPacket>
{
public:
	enum PacketType {
		Type_AnnounceMessage = 2,
		Type_InitMessage = 3,
		Type_AuthServerChallengeMessage = 4,
		Type_AuthClientResponseMessage = 5,
		Type_AuthServerVerificationMessage = 6,
		Type_InitAckMessage = 7,
		Type_AvatarRequestMessage = 8,
		Type_AvatarHeaderMessage = 9,
		Type_AvatarDataMessage = 10,
		Type_AvatarEndMessage = 11,
		Type_UnknownAvatarMessage = 12,
		Type_PlayerListMessage = 13,
		Type_GameListNewMessage = 14,
		Type_GameListUpdateMessage = 15,
		Type_GameListPlayerJoinedMessage = 16,
		Type_GameListPlayerLeftMessage = 17,
		Type_GameListAdminChangedMessage = 18,
		Type_PlayerInfoRequestMessage = 19,
		Type_PlayerInfoReplyMessage = 20,
		Type_SubscriptionRequestMessage = 21,
		Type_JoinExistingGameMessage = 22,
		Type_JoinNewGameMessage = 23,
		Type_RejoinExistingGameMessage = 24,
		Type_JoinGameAckMessage = 25,
		Type_JoinGameFailedMessage = 26,
		Type_GamePlayerJoinedMessage = 27,
		Type_GamePlayerLeftMessage = 28,
		Type_GameAdminChangedMessage = 29,
		Type_RemovedFromGameMessage = 30,
		Type_KickPlayerRequestMessage = 31,
		Type_LeaveGameRequestMessage = 32,
		Type_InvitePlayerToGameMessage = 33,
		Type_InviteNotifyMessage = 34,
		Type_RejectGameInvitationMessage = 35,
		Type_RejectInvNotifyMessage = 36,
		Type_StartEventMessage = 37,
		Type_StartEventAckMessage = 38,
		Type_GameStartInitialMessage = 39,
		Type_GameStartRejoinMessage = 40,
		Type_HandStartMessage = 41,
		Type_PlayersTurnMessage = 42,
		Type_MyActionRequestMessage = 43,
		Type_YourActionRejectedMessage = 44,
		Type_PlayersActionDoneMessage = 45,
		Type_DealFlopCardsMessage = 46,
		Type_DealTurnCardMessage = 47,
		Type_DealRiverCardMessage = 48,
		Type_AllInShowCardsMessage = 49,
		Type_EndOfHandShowCardsMessage = 50,
		Type_EndOfHandHideCardsMessage = 51,
		Type_ShowMyCardsRequestMessage = 52,
		Type_AfterHandShowCardsMessage = 53,
		Type_EndOfGameMessage = 54,
		Type_PlayerIdChangedMessage = 55,
		Type_AskKickPlayerMessage = 56,
		Type_AskKickDeniedMessage = 57,
		Type_StartKickPetitionMessage = 58,
		Type_VoteKickRequestMessage = 59,
		Type_VoteKickReplyMessage = 60,
		Type_KickPetitionUpdateMessage = 61,
		Type_EndKickPetitionMessage = 62,
		Type_StatisticsMessage = 63,
		Type_ChatRequestMessage = 64,
		Type_ChatMessage = 65,
		Type_ChatRejectMessage = 66,
		Type_DialogMessage = 67,
		Type_TimeoutWarningMessage = 68,
		Type_ResetTimeoutMessage = 69,
		Type_ReportAvatarMessage = 70,
		Type_ReportAvatarAckMessage = 71,
		Type_ReportGameMessage = 72,
		Type_ReportGameAckMessage = 73,
		Type_ErrorMessage = 74,
		Type_AdminRemoveGameMessage = 75,
		Type_AdminRemoveGameAckMessage = 76,
		Type_AdminBanPlayerMessage = 77,
		Type_AdminBanPlayerAckMessage = 78,
		Type_GameListSpectatorJoinedMessage = 79,
		Type_GameListSpectatorLeftMessage = 80,
		Type_GameSpectatorJoinedMessage = 81,
		Type_GameSpectatorLeftMessage = 82
	};

	NetPacket() {}
	~NetPacket() {}

	void SetMessage(NetMessage* m) { m_msg = m; }
	template <class T> T& Create() { return m_msg.Create<T>(); }
	NetMessage* GetMessage() { return ~m_msg; }
	const NetMessage* GetMessage() const { return ~m_msg; }
	
	int GetType() const { return m_msg ? m_msg->GetType() : 0; }

	// Serialization
	void Serialize(Stream& s);

	// Helpers
	static void SetGameData(const GameData &inData, NetGameInfoMsg &outData);
	static void GetGameData(const NetGameInfoMsg &inData, GameData &outData);

private:
	One<NetMessage> m_msg;
};

typedef std::vector<std::shared_ptr<NetPacket>> NetPacketList;

END_UPP_NAMESPACE

#endif