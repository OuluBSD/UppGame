#ifndef UPP_NETWORKING_H
#define UPP_NETWORKING_H

#include <Core/Core.h>
#include <Socket/Socket.h>  // U++ networking package
#include <Core/Time.h>
#include <Core/Thread.h>
#include <Core/Value.h>

NAMESPACE_UPP

// Network message types
enum class NetworkMessageType {
    UNKNOWN,
    CONNECT,
    DISCONNECT,
    PING,
    PONG,
    CHAT_MESSAGE,
    GAME_STATE_UPDATE,
    PLAYER_INPUT,
    CUSTOM
};

// Network message structure
struct NetworkMessage {
    NetworkMessageType type = NetworkMessageType::UNKNOWN;
    int senderId = -1;
    int targetId = -1;
    String content;
    ValueMap metadata;  // Additional data that doesn't need to be in content
    Time timestamp;

    NetworkMessage() : timestamp(GetSysTime()) {}
    NetworkMessage(NetworkMessageType type, const String& content)
        : type(type), content(content), timestamp(GetSysTime()) {}
};

// Base network interface
class NetworkInterface {
public:
    virtual ~NetworkInterface() = default;

    // Initialize the network interface
    virtual bool Initialize() = 0;

    // Send a message
    virtual bool SendMessage(const NetworkMessage& msg) = 0;

    // Receive a message (non-blocking)
    virtual bool ReceiveMessage(NetworkMessage& msg) = 0;

    // Get connection status
    virtual bool IsConnected() const = 0;

    // Get network statistics
    virtual ValueMap GetStats() const = 0;
};

// TCP Client implementation
class TcpClient : public NetworkInterface {
public:
    TcpClient();
    virtual ~TcpClient();

    bool Initialize() override;
    bool Connect(const String& host, int port);
    bool Disconnect();

    bool SendMessage(const NetworkMessage& msg) override;
    bool ReceiveMessage(NetworkMessage& msg) override;
    bool IsConnected() const override { return connected; }

    // Send a raw message (for internal use)
    bool SendRaw(const String& data);
    
    ValueMap GetStats() const override;

private:
    TcpSocket socket;
    bool connected = false;
    bool initialized = false;
    String host;
    int port = 0;
    
    // Statistics
    int64 bytesSent = 0;
    int64 bytesReceived = 0;
    int messageCountSent = 0;
    int messageCountReceived = 0;
    
    // For message serialization
    String SerializeMessage(const NetworkMessage& msg);
    NetworkMessage DeserializeMessage(const String& data);
};

// TCP Server implementation
class TcpServer : public NetworkInterface {
public:
    TcpServer();
    virtual ~TcpServer();

    bool Initialize() override;
    bool Listen(int port, int maxConnections = 10);
    bool Stop();

    bool SendMessage(const NetworkMessage& msg) override;
    bool ReceiveMessage(NetworkMessage& msg) override;
    bool IsConnected() const override { return listening; }

    // Accept a new client connection
    bool AcceptClient(TcpSocket& clientSocket);

    // Send message to specific client
    bool SendMessageTo(int clientId, const NetworkMessage& msg);

    ValueMap GetStats() const override;

private:
    TcpSocket serverSocket;
    bool listening = false;
    bool initialized = false;
    int port = 0;
    int maxConnections = 0;
    
    // Connected clients
    Vector<TcpSocket> clients;
    Vector<int> clientIds;
    
    // Statistics
    int64 bytesSent = 0;
    int64 bytesReceived = 0;
    int messageCountSent = 0;
    int messageCountReceived = 0;
};

// HTTP Client implementation (for web requests, API calls, etc.)
class HttpClient {
public:
    HttpClient();
    virtual ~HttpClient();

    // Perform a GET request
    bool Get(const String& url, String& response);

    // Perform a POST request
    bool Post(const String& url, const String& data, String& response);

    // Perform a PUT request
    bool Put(const String& url, const String& data, String& response);

    // Perform a DELETE request
    bool Delete(const String& url, String& response);

    // Set custom headers
    void SetHeader(const String& name, const String& value);

    // Get response headers
    const VectorMap<String, String>& GetResponseHeaders() const { return responseHeaders; }

    // Get status code of last request
    int GetLastStatusCode() const { return lastStatusCode; }

private:
    VectorMap<String, String> headers;  // Request headers
    VectorMap<String, String> responseHeaders;  // Response headers
    int lastStatusCode = 0;

    // Helper to format headers
    String FormatHeaders() const;
};

// WebSocket support
class WebSocket : public NetworkInterface {
public:
    WebSocket();
    virtual ~WebSocket();

    bool Initialize() override;
    bool Connect(const String& url);
    bool Disconnect();

    bool SendMessage(const NetworkMessage& msg) override;
    bool ReceiveMessage(NetworkMessage& msg) override;
    bool IsConnected() const override { return connected; }

    ValueMap GetStats() const override;

private:
    bool connected = false;
    bool initialized = false;
    String url;
    
    // WebSocket uses a regular TcpSocket but with WebSocket protocol
    TcpSocket socket;
    
    // Statistics
    int64 bytesSent = 0;
    int64 bytesReceived = 0;
    int messageCountSent = 0;
    int messageCountReceived = 0;

    // WebSocket specific functions
    bool Handshake(const String& url);
    String CreateFrame(const String& data);
    String ParseFrame(const String& frame);
};

// Network manager to coordinate multiple network interfaces
class NetworkManager {
public:
    NetworkManager();
    virtual ~NetworkManager();

    // Initialize the network manager
    bool Initialize();

    // Add a network interface
    void AddInterface(std::shared_ptr<NetworkInterface> interface);

    // Get interface by index
    std::shared_ptr<NetworkInterface> GetInterface(int index);

    // Update network interfaces (to be called regularly)
    void Update();

    // Get the number of interfaces
    int GetInterfaceCount() const { return interfaces.GetCount(); }

    // Handle network events
    std::function<void(const NetworkMessage&)> OnMessageReceived;

private:
    Vector<std::shared_ptr<NetworkInterface>> interfaces;
    bool initialized = false;
};

// Game-specific networking utilities
class GameNetworkUtils {
public:
    // Serialize a game state to a message
    static NetworkMessage SerializeGameState(const Value& gameState);

    // Deserialize a game state from a message
    static Value DeserializeGameState(const NetworkMessage& msg);

    // Serialize player input to a message
    static NetworkMessage SerializePlayerInput(const Value& input);

    // Deserialize player input from a message
    static Value DeserializePlayerInput(const NetworkMessage& msg);
};

END_UPP_NAMESPACE

#endif