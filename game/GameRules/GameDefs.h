#ifndef _CardEngine_GameDefs_h_
#define _CardEngine_GameDefs_h_

#include <Core/Core.h>

#ifdef PLATFORM_WIN32
#ifndef FAR
#define FAR
#endif
#endif

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#else
#include <winsock2.h>
#endif

NAMESPACE_UPP

typedef uint64 SessionId;
#define INVALID_SESSION_ID (SessionId)0

#define MIN_NUMBER_OF_PLAYERS		2
#define MAX_NUMBER_OF_PLAYERS		10

#define RANKING_GAME_START_CASH 10000
#define RANKING_GAME_NUMBER_OF_PLAYERS 10
#define RANKING_GAME_START_SBLIND 50

// PKRTcpSocket wrapper using raw SOCKET
class PKRTcpSocket {
public:
	PKRTcpSocket() : m_sock(INVALID_SOCKET) {}
	~PKRTcpSocket() { Close(); }
	
	bool Connect(const char *host, int port) {
		Close();
		struct hostent *server = gethostbyname(host);
		if (!server) return false;
		m_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (m_sock == INVALID_SOCKET) return false;
		struct sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
		serv_addr.sin_port = htons(port);
		if (connect(m_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
			Close();
			return false;
		}
		return true;
	}
	
	void Close() {
		if (m_sock != INVALID_SOCKET) {
#ifndef _WIN32
			close(m_sock);
#else
			closesocket(m_sock);
#endif
			m_sock = INVALID_SOCKET;
		}
	}
	
	bool IsOpen() const { return m_sock != INVALID_SOCKET; }
	
	bool Wait(dword events, int timeout_ms) {
		if (m_sock == INVALID_SOCKET) return false;
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(m_sock, &fds);
		struct timeval tv;
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;
		int r = select((int)m_sock + 1, (events & WAIT_READ ? &fds : NULL), (events & WAIT_WRITE ? &fds : NULL), NULL, &tv);
		return r > 0;
	}
	
	int Send(const void *buffer, int maxlen) {
		return send(m_sock, (const char*)buffer, maxlen, 0);
	}
	
	int Recv(void *buffer, int maxlen) {
		return recv(m_sock, (char*)buffer, maxlen, 0);
	}
	
	void Attach(SOCKET s) { Close(); m_sock = s; }
	SOCKET Detach() { SOCKET s = m_sock; m_sock = INVALID_SOCKET; return s; }
	
	String GetPeerAddr() const {
		struct sockaddr_in addr;
		socklen_t len = sizeof(addr);
		if (getpeername(m_sock, (struct sockaddr *)&addr, &len) == 0) {
			return inet_ntoa(addr.sin_addr);
		}
		return "";
	}
	
	void Put(const String& s) { Send(~s, s.GetCount()); }

private:
	SOCKET m_sock;
};

// Socket or connection related errors.
#define ERR_SOCK_INTERNAL				1
#define ERR_SOCK_SERVERADDR_NOT_SET		2
#define ERR_SOCK_INVALID_PORT			3
#define ERR_SOCK_CREATION_FAILED		4
#define ERR_SOCK_SET_ADDR_FAILED		5
#define ERR_SOCK_SET_PORT_FAILED		6
#define ERR_SOCK_RESOLVE_FAILED			7
#define ERR_SOCK_BIND_FAILED			8
#define ERR_SOCK_LISTEN_FAILED			9
#define ERR_SOCK_ACCEPT_FAILED			10
#define ERR_SOCK_CONNECT_FAILED			11
#define ERR_SOCK_CONNECT_TIMEOUT		12
#define ERR_SOCK_SELECT_FAILED			13
#define ERR_SOCK_RECV_FAILED			14
#define ERR_SOCK_SEND_FAILED			15
#define ERR_SOCK_CONN_RESET				16
#define ERR_SOCK_CONN_EXISTS			17
#define ERR_SOCK_INVALID_PACKET			18
#define ERR_SOCK_INVALID_STATE			19
#define ERR_SOCK_INVALID_TYPE			20
#define ERR_SOCK_INVALID_SERVERLIST_URL	21
#define ERR_SOCK_INVALID_SERVERLIST_XML 22
#define ERR_SOCK_UNZIP_FAILED			23
#define ERR_SOCK_TRANSFER_INIT_FAILED	24
#define ERR_SOCK_TRANSFER_OPEN_FAILED	25
#define ERR_SOCK_TRANSFER_INVALID_URL	26
#define ERR_SOCK_TRANSFER_SELECT_FAILED	27
#define ERR_SOCK_TRANSFER_FAILED		28
#define ERR_SOCK_CONNECT_IPV6_FAILED	29
#define ERR_SOCK_CONNECT_IPV6_TIMEOUT	30

// Game errors.
#define ERR_NET_VERSION_NOT_SUPPORTED	101
#define ERR_NET_SERVER_MAINTENANCE		102
#define ERR_NET_SERVER_FULL				103
#define ERR_NET_INVALID_PASSWORD		104
#define ERR_NET_INVALID_PASSWORD_STR	105
#define ERR_NET_PLAYER_NAME_IN_USE		106
#define ERR_NET_INVALID_PLAYER_NAME		107
#define ERR_NET_INVALID_PLAYER_CARDS	108
#define ERR_NET_INVALID_PLAYER_RESULTS	109
#define ERR_NET_INVALID_GAME_NAME		110
#define ERR_NET_INVALID_GAME_ROUND		111
#define ERR_NET_INVALID_SESSION			112
#define ERR_NET_UNKNOWN_GAME			113
#define ERR_NET_INVALID_CHAT_TEXT		114
#define ERR_NET_UNKNOWN_PLAYER_ID		115
#define ERR_NET_NO_CURRENT_PLAYER		116
#define ERR_NET_PLAYER_NOT_ACTIVE		117
#define ERR_NET_PLAYER_KICKED			118
#define ERR_NET_PLAYER_BANNED			119
#define ERR_NET_PLAYER_BLOCKED			120
#define ERR_NET_SESSION_TIMED_OUT		121
#define ERR_NET_INVALID_PLAYER_COUNT	122
#define ERR_NET_TOO_MANY_MANUAL_BLINDS	123
#define ERR_NET_INVALID_AVATAR_FILE		124
#define ERR_NET_AVATAR_TOO_LARGE		125
#define ERR_NET_BUF_INVALID_SIZE		126
#define ERR_NET_INVALID_REQUEST_ID		127
#define ERR_NET_WRONG_AVATAR_SIZE		128
#define ERR_NET_START_TIMEOUT			129
#define ERR_NET_GAME_TERMINATION_FAILED	130
#define ERR_NET_INTERNAL_GAME_ERROR		131
#define ERR_NET_DEALER_NOT_FOUND		132
#define ERR_NET_INIT_BLOCKED			133
#define ERR_NET_GSASL_INIT_FAILED		134
#define ERR_NET_GSASL_NO_SCRAM			135
#define ERR_NET_DB_CONNECT_FAILED		136

// Engine errors -> LocalExceptions
#define ERR_SEAT_NOT_FOUND				10001
#define ERR_ACTIVE_PLAYER_NOT_FOUND			10002
#define ERR_RUNNING_PLAYER_NOT_FOUND			10003
#define ERR_DEALER_NOT_FOUND			10004
#define ERR_CURRENT_PLAYER_NOT_FOUND			10005
#define ERR_NEXT_DEALER_NOT_FOUND			10010
#define ERR_NEXT_ACTIVE_PLAYER_NOT_FOUND		10011
#define ERR_FORMER_RUNNING_PLAYER_NOT_FOUND		10012
#define ERR_NO_WINNER					10020

// Notifications - removed from game
#define NTF_NET_INTERNAL				201
#define NTF_NET_REMOVED_ON_REQUEST		202
#define NTF_NET_REMOVED_GAME_FULL		203
#define NTF_NET_REMOVED_ALREADY_RUNNING	204
#define NTF_NET_REMOVED_KICKED			205
#define NTF_NET_REMOVED_TIMEOUT			206
#define NTF_NET_REMOVED_START_FAILED	207
#define NTF_NET_REMOVED_GAME_CLOSED		208

// Notifications - join failed
#define NTF_NET_JOIN_GAME_INVALID		210
#define NTF_NET_JOIN_GAME_FULL			211
#define NTF_NET_JOIN_ALREADY_RUNNING	212
#define NTF_NET_JOIN_INVALID_PASSWORD	213
#define NTF_NET_JOIN_GUEST_FORBIDDEN	214
#define NTF_NET_JOIN_NOT_INVITED		215
#define NTF_NET_JOIN_GAME_NAME_IN_USE	216
#define NTF_NET_JOIN_GAME_BAD_NAME		217
#define NTF_NET_JOIN_INVALID_SETTINGS	218
#define NTF_NET_JOIN_IP_BLOCKED			219
#define NTF_NET_JOIN_REJOIN_FAILED		220
#define NTF_NET_JOIN_NO_SPECTATORS		221

enum AiBackend {
	AI_BACKEND_POKERTH = 0
};

enum ServerMode {
	SERVER_MODE_LAN = 1,
	SERVER_MODE_INTERNET
};

enum TexasRound {
	GAME_STATE_PREFLOP = 0,
	GAME_STATE_FLOP = 1,
	GAME_STATE_TURN = 2,
	GAME_STATE_RIVER = 3,
	GAME_STATE_POST_RIVER = 4
};

enum HeartsRound {
	HEARTS_STATE_PASSING = 0,
	HEARTS_STATE_TRICKS = 1,
	HEARTS_STATE_POST_GAME = 2
};

enum PlayerAction {
	PLAYER_ACTION_NONE = 0,
	PLAYER_ACTION_CHECK,
	PLAYER_ACTION_CALL,
	PLAYER_ACTION_BET,
	PLAYER_ACTION_RAISE,
	PLAYER_ACTION_FOLD,
	PLAYER_ACTION_ALLIN,
	PLAYER_ACTION_SMALL_BLIND,
	PLAYER_ACTION_BIG_BLIND,
	PLAYER_ACTION_PASS_CARDS,
	PLAYER_ACTION_PLAY_CARD
};

enum PlayerActionLog {
	LOG_ACTION_NONE = 0,
	LOG_ACTION_DEALER,
	LOG_ACTION_SMALL_BLIND,
	LOG_ACTION_BIG_BLIND,
	LOG_ACTION_FOLD,
	LOG_ACTION_CHECK,
	LOG_ACTION_CALL,
	LOG_ACTION_BET,
	LOG_ACTION_ALL_IN,
	LOG_ACTION_SHOW,
	LOG_ACTION_HAS,
	LOG_ACTION_WIN,
	LOG_ACTION_WIN_SIDE_POT,
	LOG_ACTION_SIT_OUT,
	LOG_ACTION_WIN_GAME,
	LOG_ACTION_LEFT,
	LOG_ACTION_KICKED,
	LOG_ACTION_ADMIN,
	LOG_ACTION_JOIN
};

enum DenyGameInvitationReason {
	DENY_GAME_INVITATION_NO = 0,
	DENY_GAME_INVITATION_BUSY
};

enum GuiButtonId {
	GBUTTON_NONE = 0,
	GBUTTON_DEALER = 1,
	GBUTTON_SMALL_BLIND = 2,
	GBUTTON_BIG_BLIND = 3
};

typedef unsigned DB_id;
#define DB_ID_INVALID (DB_id)0

END_UPP_NAMESPACE

#endif