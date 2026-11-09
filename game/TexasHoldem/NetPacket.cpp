#include "NetPacket.h"

NAMESPACE_UPP

// --- Serialization Implementations ---

void AnnounceMsg::Serialize(Stream& s) {
	s % protocolVersion % latestGameVersion % latestBetaRevision % serverType % numPlayersOnServer;
}

void InitMsg::Serialize(Stream& s) {
	s % requestedVersion % buildId % myLastSessionId % authServerPassword % login % nickName % clientUserData % avatarHash;
}

void AuthServerChallengeMsg::Serialize(Stream& s) {
	s % challenge;
}

void AuthClientResponseMsg::Serialize(Stream& s) {
	s % response;
}

void AuthServerVerificationMsg::Serialize(Stream& s) {
	s % verified;
}

void InitAckMsg::Serialize(Stream& s) {
	s % yourSessionId % yourPlayerId % yourAvatarHash % rejoinGameId;
}

void AvatarRequestMsg::Serialize(Stream& s) {
	s % playerId % avatarHash;
}

void AvatarHeaderMsg::Serialize(Stream& s) {
	s % playerId % avatarHash % avatarType % totalSize;
}

void AvatarDataMsg::Serialize(Stream& s) {
	s % playerId % data;
}

void AvatarEndMsg::Serialize(Stream& s) {
	s % playerId;
}

void PlayerListMsg::Serialize(Stream& s) {
	s % players;
}

void GameListNewMsg::Serialize(Stream& s) {
	s % games;
}

void GameListUpdateMsg::Serialize(Stream& s) {
	s % gameId % players % maxPlayers % state;
}

void GameListPlayerJoinedMsg::Serialize(Stream& s) {
	s % gameId % playerId;
}

void GameListPlayerLeftMsg::Serialize(Stream& s) {
	s % gameId % playerId;
}

void PlayerInfoRequestMsg::Serialize(Stream& s) {
	s % playerId;
}

void PlayerInfoReplyMsg::Serialize(Stream& s) {
	s % playerId % name % rights % avatarHash % gamesPlayed % gamesWon;
}

void JoinExistingGameMsg::Serialize(Stream& s) {
	s % gameId % gamePassword % autoLeave;
}

void JoinNewGameMsg::Serialize(Stream& s) {
	s % gameInfo % gamePassword % autoLeave;
}

void JoinGameAckMsg::Serialize(Stream& s) {
	s % gameId % gameInfo % isAdmin;
}

void JoinGameFailedMsg::Serialize(Stream& s) {
	s % gameId % reason;
}

void GamePlayerJoinedMsg::Serialize(Stream& s) {
	s % gameId % playerId % playerName;
}

void GamePlayerLeftMsg::Serialize(Stream& s) {
	s % gameId % playerId;
}

void KickPlayerRequestMsg::Serialize(Stream& s) {
	s % gameId % playerId;
}

void LeaveGameRequestMsg::Serialize(Stream& s) {
	s % gameId;
}

void GameStartInitialMsg::Serialize(Stream& s) {
	s % gameId % playerIds % playerNames % playerMoney;
}

void HandStartMsg::Serialize(Stream& s) {
	s % gameId % smallBlind % seatStates % dealerPlayerId;
	bool hasCards = !plainCards.IsEmpty();
	s % hasCards;
	if (hasCards) {
		if (s.IsLoading()) plainCards.Create();
		plainCards->Serialize(s);
	}
	s % encryptedCards;
}

void PlayersTurnMsg::Serialize(Stream& s) {
	s % gameId % playerId % gameState;
}

void MyActionRequestMsg::Serialize(Stream& s) {
	s % gameId % handNum % gameState % myAction % myRelativeBet;
}

void PlayersActionDoneMsg::Serialize(Stream& s) {
	s % gameId % playerId % gameState % playerAction % totalPlayerBet % playerMoney % highestSet % minimumRaise;
}

void DealFlopCardsMsg::Serialize(Stream& s) {
	s % gameId % card1 % card2 % card3;
}

void DealTurnCardMsg::Serialize(Stream& s) {
	s % gameId % card;
}

void DealRiverCardMsg::Serialize(Stream& s) {
	s % gameId % card;
}

void AllInShowCardsMsg::Serialize(Stream& s) {
	s % gameId % playersAllIn;
}

void EndOfHandShowCardsMsg::Serialize(Stream& s) {
	s % gameId % playerResults;
}

void EndOfHandHideCardsMsg::Serialize(Stream& s) {
	s % gameId % playerId % moneyWon % playerMoney;
}

void EndOfGameMsg::Serialize(Stream& s) {
	s % gameId % winnerPlayerId;
}

void AskKickPlayerMsg::Serialize(Stream& s) {
	s % gameId % playerId;
}

void VoteKickRequestMsg::Serialize(Stream& s) {
	s % gameId % targetPlayerId % reason;
}

void VoteKickReplyMsg::Serialize(Stream& s) {
	s % gameId % vote;
}

void ChatMsg::Serialize(Stream& s) {
	s % gameId % playerId % chatType % chatText;
}

void ErrorMsg::Serialize(Stream& s) {
	s % errorReason;
}

// --- Main Serialize Dispatch ---

void NetPacket::Serialize(Stream& s)
{
	int type = GetType();
	s % type;
	if (s.IsLoading()) {
		switch (type) {
		case Type_AnnounceMessage: m_msg.Create<AnnounceMsg>(); break;
		case Type_InitMessage: m_msg.Create<InitMsg>(); break;
		case Type_AuthServerChallengeMessage: m_msg.Create<AuthServerChallengeMsg>(); break;
		case Type_AuthClientResponseMessage: m_msg.Create<AuthClientResponseMsg>(); break;
		case Type_AuthServerVerificationMessage: m_msg.Create<AuthServerVerificationMsg>(); break;
		case Type_InitAckMessage: m_msg.Create<InitAckMsg>(); break;
		case Type_AvatarRequestMessage: m_msg.Create<AvatarRequestMsg>(); break;
		case Type_AvatarHeaderMessage: m_msg.Create<AvatarHeaderMsg>(); break;
		case Type_AvatarDataMessage: m_msg.Create<AvatarDataMsg>(); break;
		case Type_AvatarEndMessage: m_msg.Create<AvatarEndMsg>(); break;
		
		case Type_PlayerListMessage: m_msg.Create<PlayerListMsg>(); break;
		case Type_GameListNewMessage: m_msg.Create<GameListNewMsg>(); break;
		case Type_GameListUpdateMessage: m_msg.Create<GameListUpdateMsg>(); break;
		case Type_GameListPlayerJoinedMessage: m_msg.Create<GameListPlayerJoinedMsg>(); break;
		case Type_GameListPlayerLeftMessage: m_msg.Create<GameListPlayerLeftMsg>(); break;
		
		case Type_PlayerInfoRequestMessage: m_msg.Create<PlayerInfoRequestMsg>(); break;
		case Type_PlayerInfoReplyMessage: m_msg.Create<PlayerInfoReplyMsg>(); break;
		
		case Type_JoinExistingGameMessage: m_msg.Create<JoinExistingGameMsg>(); break;
		case Type_JoinNewGameMessage: m_msg.Create<JoinNewGameMsg>(); break;
		case Type_JoinGameAckMessage: m_msg.Create<JoinGameAckMsg>(); break;
		case Type_JoinGameFailedMessage: m_msg.Create<JoinGameFailedMsg>(); break;
		case Type_GamePlayerJoinedMessage: m_msg.Create<GamePlayerJoinedMsg>(); break;
		case Type_GamePlayerLeftMessage: m_msg.Create<GamePlayerLeftMsg>(); break;
		case Type_KickPlayerRequestMessage: m_msg.Create<KickPlayerRequestMsg>(); break;
		case Type_LeaveGameRequestMessage: m_msg.Create<LeaveGameRequestMsg>(); break;
		
		case Type_GameStartInitialMessage: m_msg.Create<GameStartInitialMsg>(); break;
		case Type_HandStartMessage: m_msg.Create<HandStartMsg>(); break;
		case Type_PlayersTurnMessage: m_msg.Create<PlayersTurnMsg>(); break;
		case Type_MyActionRequestMessage: m_msg.Create<MyActionRequestMsg>(); break;
		case Type_PlayersActionDoneMessage: m_msg.Create<PlayersActionDoneMsg>(); break;
		case Type_DealFlopCardsMessage: m_msg.Create<DealFlopCardsMsg>(); break;
		case Type_DealTurnCardMessage: m_msg.Create<DealTurnCardMsg>(); break;
		case Type_DealRiverCardMessage: m_msg.Create<DealRiverCardMsg>(); break;
		case Type_AllInShowCardsMessage: m_msg.Create<AllInShowCardsMsg>(); break;
		case Type_EndOfHandShowCardsMessage: m_msg.Create<EndOfHandShowCardsMsg>(); break;
		case Type_EndOfHandHideCardsMessage: m_msg.Create<EndOfHandHideCardsMsg>(); break;
		case Type_EndOfGameMessage: m_msg.Create<EndOfGameMsg>(); break;
		
		case Type_AskKickPlayerMessage: m_msg.Create<AskKickPlayerMsg>(); break;
		case Type_VoteKickRequestMessage: m_msg.Create<VoteKickRequestMsg>(); break;
		case Type_VoteKickReplyMessage: m_msg.Create<VoteKickReplyMsg>(); break;
		
		case Type_ChatMessage: m_msg.Create<ChatMsg>(); break;
		case Type_ErrorMessage: m_msg.Create<ErrorMsg>(); break;
		
		default: m_msg.Clear(); break;
		}
	}
	if (m_msg) m_msg->Serialize(s);
}

// --- Helpers ---

void NetPacket::SetGameData(const GameData &inData, NetGameInfoMsg &outData)
{
	outData.netGameType = (int)inData.gameType;
	outData.allowSpectators = inData.allowSpectators;
	outData.maxNumPlayers = (uint32)inData.maxNumberOfPlayers;
	outData.raiseIntervalMode = (int)inData.raiseIntervalMode;
	if (inData.raiseIntervalMode == RAISE_ON_HANDNUMBER) {
		outData.raiseEveryHands = inData.raiseSmallBlindEveryHandsValue;
		outData.raiseEveryMinutes = 0;
	} else {
		outData.raiseEveryMinutes = inData.raiseSmallBlindEveryMinutesValue;
		outData.raiseEveryHands = 0;
	}
	outData.endRaiseMode = (int)inData.afterManualBlindsMode;
	outData.proposedGuiSpeed = inData.guiSpeed;
	outData.delayBetweenHands = inData.delayBetweenHandsSec;
	outData.playerActionTimeout = inData.playerActionTimeoutSec;
	outData.firstSmallBlind = inData.firstSmallBlind;
	outData.endRaiseSmallBlindValue = inData.afterMBAlwaysRaiseValue;
	outData.startMoney = inData.startMoney;
	outData.manualBlinds.Clear();
	for (int b : inData.manualBlindsList) outData.manualBlinds.Add((uint32)b);
}

void NetPacket::GetGameData(const NetGameInfoMsg &inData, GameData &outData)
{
	outData.gameType = (NetworkGameType)inData.netGameType;
	outData.allowSpectators = inData.allowSpectators;
	outData.maxNumberOfPlayers = (int)inData.maxNumPlayers;
	outData.raiseIntervalMode = (RaiseIntervalMode)inData.raiseIntervalMode;
	if (outData.raiseIntervalMode == RAISE_ON_HANDNUMBER)
		outData.raiseSmallBlindEveryHandsValue = inData.raiseEveryHands;
	else
		outData.raiseSmallBlindEveryMinutesValue = inData.raiseEveryMinutes;
	outData.afterManualBlindsMode = (AfterManualBlindsMode)inData.endRaiseMode;
	outData.guiSpeed = inData.proposedGuiSpeed;
	outData.delayBetweenHandsSec = inData.delayBetweenHands;
	outData.playerActionTimeoutSec = inData.playerActionTimeout;
	outData.firstSmallBlind = inData.firstSmallBlind;
	outData.afterMBAlwaysRaiseValue = inData.endRaiseSmallBlindValue;
	outData.startMoney = inData.startMoney;
	outData.manualBlindsList.clear();
	for (uint32 b : inData.manualBlinds) outData.manualBlindsList.push_back((int)b);
	outData.raiseMode = outData.manualBlindsList.empty() ? DOUBLE_BLINDS : MANUAL_BLINDS_ORDER;
}

END_UPP_NAMESPACE
