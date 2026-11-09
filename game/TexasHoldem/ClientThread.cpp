#include "ClientThread.h"
#include "NetContext.h"
#include <GameRules/Game.h>
#include <GameRules/EngineLog.h>
#include "NetPacket.h"
#include "ClientState.h"

NAMESPACE_UPP

std::shared_ptr<ClientThread> g_clientThread;

ClientThread::ClientThread(GuiInterface &gui, AvatarManager &avatarManager, EngineLog *myLog)
	: m_gui(gui), m_avatarManager(avatarManager), m_clientLog(myLog), m_curState(nullptr)
{
	m_context = std::make_shared<ClientContext>();
}

ClientThread::~ClientThread()
{
}

void ClientThread::Init(const String &serverAddress, const String &serverListUrl, const String &serverPassword,
          bool useServerList, unsigned serverPort, bool ipv6, bool sctp, bool tls,
          const String &avatarServerAddress, const String &playerName,
          const String &avatarFile, const String &cacheDir)
{
	m_context->SetServerAddr(serverAddress);
	m_context->SetServerListUrl(serverListUrl);
	m_context->SetServerPassword(serverPassword);
	m_context->SetUseServerList(useServerList);
	m_context->SetServerPort(serverPort);
	m_context->SetAddrFamily(ipv6 ? AF_INET6 : AF_INET);
	m_context->SetSctp(sctp);
	m_context->SetTls(tls);
	m_context->SetAvatarServerAddr(avatarServerAddress);
	m_context->SetPlayerName(playerName);
	m_context->SetAvatarFile(avatarFile);
	m_context->SetCacheDir(cacheDir);
}

void ClientThread::SignalTermination()
{
	ThreadWrapper::SignalTermination();
}

void ClientThread::Main()
{
	try {
		m_curState = &CLIENT_INITIAL_STATE::Instance();
		m_curState->Enter(shared_from_this());

		auto socket = std::make_shared<PKRTcpSocket>();
		if (!socket->Connect(m_context->GetServerAddr(), m_context->GetServerPort())) {
			throw Exc("Failed to connect to server.");
		}

		auto session = std::make_shared<SessionData>(0, *this);
		session->GetSocket().Attach(socket->Detach());
		m_context->SetSessionData(session);

		// Send Init
		auto initPkt = std::make_shared<NetPacket>();
		auto& init = initPkt->Create<InitMsg>();
		init.requestedVersion.major = NET_VERSION_MAJOR;
		init.requestedVersion.minor = NET_VERSION_MINOR;
		init.login = m_loginData.isGuest ? 0 : 1;
		init.nickName = m_context->GetPlayerName();
		EnqueuePacket(initPkt);

		String receiveBuffer;
		while (!ShouldTerminate()) {
			if (!session->GetSocket().IsOpen()) break;

			SendQueuedPackets();

			if (session->GetSocket().Wait(WAIT_READ, 10)) {
				char buffer[4096];
				int n = session->GetSocket().Recv(buffer, sizeof(buffer));
				if (n > 0) {
					receiveBuffer.Cat(buffer, n);
					while (receiveBuffer.GetCount() >= 4) {
						uint32 size = Peek32le(~receiveBuffer);
						if (receiveBuffer.GetCount() >= (int)(4 + size)) {
							String pktData = receiveBuffer.Mid(4, size);
							receiveBuffer.Remove(0, 4 + size);
							
							auto pkt = std::make_shared<NetPacket>();
							StringStream ss(pktData);
							pkt->Serialize(ss);
							HandlePacket(session, pkt);
						} else break;
					}
				} else if (n == 0) break;
			}
			Msleep(10);
		}
	} catch (const Exc &e) {
		if (m_clientLog) m_clientLog->flushLog();
		m_gui.SignalNetClientError(ERR_SOCK_CONNECT_FAILED, 0);
	}
}

void ClientThread::EnqueuePacket(std::shared_ptr<NetPacket> packet)
{
	Mutex::Lock lock(m_curStatsMutex);
	m_outPacketList.push_back(packet);
}

void ClientThread::SendQueuedPackets()
{
	std::vector<std::shared_ptr<NetPacket>> list;
	{
		Mutex::Lock lock(m_curStatsMutex);
		list = std::move(m_outPacketList);
		m_outPacketList.clear();
	}

	auto session = m_context->GetSessionData();
	if (session && session->GetSocket().IsOpen()) {
		for (auto& pkt : list) {
			StringStream ss;
			pkt->Serialize(ss);
			String data = ss.GetResult();
			uint32 size = data.GetCount();
			
			char header[4];
			Poke32le(header, size);
			session->GetSocket().Send(header, 4);
			session->GetSocket().Send(~data, size);
		}
	}
}

void ClientThread::InitGame()
{
}

void ClientThread::SendSessionPacket(std::shared_ptr<NetPacket> packet)
{
	EnqueuePacket(packet);
}

void ClientThread::SetGameId(unsigned id)
{
	Mutex::Lock lock(m_curGameIdMutex);
	m_curGameId = id;
}

unsigned ClientThread::GetGameId() const
{
	Mutex::Lock lock(m_curGameIdMutex);
	return m_curGameId;
}

void ClientThread::SetGuiPlayerId(unsigned guiPlayerId)
{
	Mutex::Lock lock(m_guiPlayerIdMutex);
	m_guiPlayerId = guiPlayerId;
}

unsigned ClientThread::GetGuiPlayerId() const
{
	Mutex::Lock lock(m_guiPlayerIdMutex);
	return m_guiPlayerId;
}

void ClientThread::SelectServer(unsigned serverId)
{
	Mutex::Lock lock(m_selectServerMutex);
	m_isServerSelected = true;
	m_selectedServerId = serverId;
}

void ClientThread::SetLogin(const String &userName, const String &password, bool isGuest)
{
	Mutex::Lock lock(m_loginDataMutex);
	m_loginData.userName = userName;
	m_loginData.password = password;
	m_loginData.isGuest = isGuest;
}

ServerInfo ClientThread::GetServerInfo(unsigned serverId) const
{
	Mutex::Lock lock(m_serverInfoMapMutex);
	return m_serverInfoMap.Get(serverId, ServerInfo());
}

GameInfo ClientThread::GetGameInfo(unsigned gameId) const
{
	Mutex::Lock lock(m_gameInfoMapMutex);
	return m_gameInfoMap.Get(gameId, GameInfo());
}

PlayerInfo ClientThread::GetPlayerInfo(unsigned playerId) const
{
	Mutex::Lock lock(m_playerInfoMapMutex);
	return m_playerInfoMap.Get(playerId, PlayerInfo());
}

bool ClientThread::GetPlayerIdFromName(const String &playerName, unsigned &playerId) const
{
	Mutex::Lock lock(m_playerInfoMapMutex);
	for(int i = 0; i < m_playerInfoMap.GetCount(); i++) {
		if (m_playerInfoMap[i].playerName == playerName) {
			playerId = m_playerInfoMap.GetKey(i);
			return true;
		}
	}
	return false;
}

unsigned ClientThread::GetGameIdOfPlayer(unsigned playerId) const
{
	Mutex::Lock lock(m_gameInfoMapMutex);
	for(int i = 0; i < m_gameInfoMap.GetCount(); i++) {
		for(unsigned pId : m_gameInfoMap[i].players) {
			if (pId == playerId) return m_gameInfoMap.GetKey(i);
		}
	}
	return 0;
}

ServerStats ClientThread::GetStatData() const
{
	Mutex::Lock lock(m_curStatsMutex);
	return m_curStats;
}

int ClientThread::GetOrigGuiPlayerNum() const
{
	return m_origGuiPlayerNum;
}

void ClientThread::StartAsyncRead()
{
}

void ClientThread::HandlePacket(std::shared_ptr<SessionData> session, std::shared_ptr<NetPacket> packet)
{
	int type = packet->GetType();
	if (type == NetPacket::Type_GameListNewMessage) {
		auto* msg = dynamic_cast<GameListNewMsg*>(packet->GetMessage());
		if (msg) WhenGameListNew(msg->games);
	} else if (type == NetPacket::Type_PlayerListMessage) {
		auto* msg = dynamic_cast<PlayerListMsg*>(packet->GetMessage());
		if (msg) WhenPlayerList(msg->players);
	} else if (type == NetPacket::Type_ChatMessage) {
		auto* msg = dynamic_cast<ChatMsg*>(packet->GetMessage());
		if (msg) {
			String name = "Unknown";
			{
				Mutex::Lock lock(m_playerInfoMapMutex);
				if (m_playerInfoMap.Find(msg->playerId) >= 0)
					name = m_playerInfoMap.Get(msg->playerId).playerName;
			}
			WhenChatMessage(name, msg->chatText);
		}
	} else if (type == NetPacket::Type_HandStartMessage) {
		auto* msg = dynamic_cast<HandStartMsg*>(packet->GetMessage());
		if (msg) WhenHandStart(*msg);
	} else if (type == NetPacket::Type_PlayersTurnMessage) {
		auto* msg = dynamic_cast<PlayersTurnMsg*>(packet->GetMessage());
		if (msg) WhenPlayersTurn(*msg);
	} else if (type == NetPacket::Type_PlayersActionDoneMessage) {
		auto* msg = dynamic_cast<PlayersActionDoneMsg*>(packet->GetMessage());
		if (msg) WhenPlayersActionDone(*msg);
	} else if (type == NetPacket::Type_JoinGameAckMessage) {
		auto* msg = dynamic_cast<JoinGameAckMsg*>(packet->GetMessage());
		if (msg) WhenJoinGameAck(*msg);
	} else if (type == NetPacket::Type_GamePlayerJoinedMessage) {
		auto* msg = dynamic_cast<GamePlayerJoinedMsg*>(packet->GetMessage());
		if (msg) WhenGamePlayerJoined(*msg);
	} else if (type == NetPacket::Type_GamePlayerLeftMessage) {
		auto* msg = dynamic_cast<GamePlayerLeftMsg*>(packet->GetMessage());
		if (msg) WhenGamePlayerLeft(*msg);
	} else if (type == NetPacket::Type_GameStartInitialMessage) {
		auto* msg = dynamic_cast<GameStartInitialMsg*>(packet->GetMessage());
		if (msg) WhenGameStartInitial(*msg);
	} else if (type == NetPacket::Type_EndOfHandShowCardsMessage) {
		auto* msg = dynamic_cast<EndOfHandShowCardsMsg*>(packet->GetMessage());
		if (msg) WhenEndOfHandShowCards(*msg);
	} else if (type == NetPacket::Type_EndOfHandHideCardsMessage) {
		auto* msg = dynamic_cast<EndOfHandHideCardsMsg*>(packet->GetMessage());
		if (msg) WhenEndOfHandHideCards(*msg);
	}

	if (m_curState) m_curState->HandlePacket(shared_from_this(), packet);
}

void ClientThread::SessionError(std::shared_ptr<SessionData> session, int errorCode)
{
}

END_UPP_NAMESPACE
