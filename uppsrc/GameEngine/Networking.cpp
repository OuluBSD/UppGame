#include "Networking.h"
#include <plugin/json/Json.h>  // For JSON serialization

NAMESPACE_UPP

// TcpClient implementation
TcpClient::TcpClient() {
}

TcpClient::~TcpClient() {
    Disconnect();
}

bool TcpClient::Initialize() {
    initialized = true;
    return true;
}

bool TcpClient::Connect(const String& host, int port) {
    if (!initialized) {
        return false;
    }

    this->host = host;
    this->port = port;

    if (!socket.Open()) {
        return false;
    }

    if (!socket.Connect(host, port)) {
        socket.Close();
        return false;
    }

    connected = true;
    return true;
}

bool TcpClient::Disconnect() {
    connected = false;
    if (socket.IsOpen()) {
        socket.Close();
    }
    return true;
}

bool TcpClient::SendMessage(const NetworkMessage& msg) {
    if (!connected) {
        return false;
    }

    String serialized = SerializeMessage(msg);
    if (socket.Put(serialized)) {
        bytesSent += serialized.GetLength();
        messageCountSent++;
        return true;
    }
    return false;
}

bool TcpClient::ReceiveMessage(NetworkMessage& msg) {
    if (!connected) {
        return false;
    }

    String data;
    if (socket.Get(data)) {
        bytesReceived += data.GetLength();
        messageCountReceived++;
        
        msg = DeserializeMessage(data);
        return true;
    }
    return false;
}

bool TcpClient::SendRaw(const String& data) {
    if (!connected) {
        return false;
    }
    
    if (socket.Put(data)) {
        bytesSent += data.GetLength();
        return true;
    }
    return false;
}

ValueMap TcpClient::GetStats() const {
    ValueMap stats;
    stats.Add("bytes_sent", bytesSent);
    stats.Add("bytes_received", bytesReceived);
    stats.Add("messages_sent", messageCountSent);
    stats.Add("messages_received", messageCountReceived);
    stats.Add("connected", connected);
    return stats;
}

String TcpClient::SerializeMessage(const NetworkMessage& msg) {
    // Simple serialization - in practice, you might want a more efficient format
    JsonBuilder builder;
    builder("type", int(msg.type))
           ("sender_id", msg.senderId)
           ("target_id", msg.targetId)
           ("content", msg.content)
           ("timestamp", msg.timestamp.ToString());
    
    // Add metadata if present
    if (!msg.metadata.IsEmpty()) {
        builder("metadata", msg.metadata);
    }
    
    return builder.ToString();
}

NetworkMessage TcpClient::DeserializeMessage(const String& data) {
    NetworkMessage msg;
    
    try {
        JsonParser parser(data);
        JsonValue json = parser;
        
        if (json.IsObject()) {
            msg.type = NetworkMessageType(int(json("type", 0)));
            msg.senderId = json("sender_id", -1);
            msg.targetId = json("target_id", -1);
            msg.content = json("content", "");
            String timestampStr = json("timestamp", "");
            if (!timestampStr.IsEmpty()) {
                msg.timestamp = ScanTime(timestampStr);
            }
            
            JsonValue metadataJson = json("metadata");
            if (metadataJson.IsObject()) {
                // Convert JSON object to ValueMap (simplified)
                // A full implementation would recursively convert the JSON
            }
        }
    }
    catch (...) {
        // If parsing fails, return an empty message
    }
    
    return msg;
}

// TcpServer implementation
TcpServer::TcpServer() {
}

TcpServer::~TcpServer() {
    Stop();
}

bool TcpServer::Initialize() {
    initialized = true;
    return true;
}

bool TcpServer::Listen(int port, int maxConnections) {
    if (!initialized) {
        return false;
    }

    this->port = port;
    this->maxConnections = maxConnections;

    if (!serverSocket.Open()) {
        return false;
    }

    if (!serverSocket.Listen(port, maxConnections)) {
        serverSocket.Close();
        return false;
    }

    listening = true;
    return true;
}

bool TcpServer::Stop() {
    listening = false;
    if (serverSocket.IsOpen()) {
        serverSocket.Close();
    }
    
    // Close all client connections
    for (auto& client : clients) {
        if (client.IsOpen()) {
            client.Close();
        }
    }
    clients.Clear();
    clientIds.Clear();
    
    return true;
}

bool TcpServer::SendMessage(const NetworkMessage& msg) {
    // Send to all connected clients
    bool success = true;
    for (int i = 0; i < clients.GetCount(); i++) {
        if (clients[i].IsOpen()) {
            String serialized = SerializeMessage(msg);
            if (!clients[i].Put(serialized)) {
                success = false;
            } else {
                bytesSent += serialized.GetLength();
                messageCountSent++;
            }
        }
    }
    return success;
}

bool TcpServer::SendMessageTo(int clientId, const NetworkMessage& msg) {
    // Find the client with the specified ID
    for (int i = 0; i < clientIds.GetCount(); i++) {
        if (clientIds[i] == clientId && clients[i].IsOpen()) {
            String serialized = SerializeMessage(msg);
            if (clients[i].Put(serialized)) {
                bytesSent += serialized.GetLength();
                messageCountSent++;
                return true;
            }
        }
    }
    return false;
}

bool TcpServer::AcceptClient(TcpSocket& clientSocket) {
    if (!listening || !serverSocket.IsOpen()) {
        return false;
    }

    if (serverSocket.Accept(clientSocket)) {
        // Assign a new ID to this client
        int newId = clients.GetCount() > 0 ? clientIds.Top() + 1 : 1;
        clientIds.Add(newId);
        clients.Add(pick(clientSocket));
        return true;
    }
    
    return false;
}

bool TcpServer::ReceiveMessage(NetworkMessage& msg) {
    // Check all clients for incoming messages
    for (int i = 0; i < clients.GetCount(); i++) {
        if (clients[i].IsOpen()) {
            String data;
            if (clients[i].Get(data)) {
                bytesReceived += data.GetLength();
                messageCountReceived++;
                
                msg = DeserializeMessage(data);
                msg.senderId = clientIds[i];  // Set sender ID
                return true;
            }
        }
    }
    return false;
}

ValueMap TcpServer::GetStats() const {
    ValueMap stats;
    stats.Add("bytes_sent", bytesSent);
    stats.Add("bytes_received", bytesReceived);
    stats.Add("messages_sent", messageCountSent);
    stats.Add("messages_received", messageCountReceived);
    stats.Add("connected_clients", clients.GetCount());
    stats.Add("listening", listening);
    return stats;
}

String TcpServer::SerializeMessage(const NetworkMessage& msg) {
    JsonBuilder builder;
    builder("type", int(msg.type))
           ("sender_id", msg.senderId)
           ("target_id", msg.targetId)
           ("content", msg.content)
           ("timestamp", msg.timestamp.ToString());
    
    // Add metadata if present
    if (!msg.metadata.IsEmpty()) {
        builder("metadata", msg.metadata);
    }
    
    return builder.ToString();
}

NetworkMessage TcpServer::DeserializeMessage(const String& data) {
    NetworkMessage msg;
    
    try {
        JsonParser parser(data);
        JsonValue json = parser;
        
        if (json.IsObject()) {
            msg.type = NetworkMessageType(int(json("type", 0)));
            msg.senderId = json("sender_id", -1);
            msg.targetId = json("target_id", -1);
            msg.content = json("content", "");
            String timestampStr = json("timestamp", "");
            if (!timestampStr.IsEmpty()) {
                msg.timestamp = ScanTime(timestampStr);
            }
        }
    }
    catch (...) {
        // If parsing fails, return an empty message
    }
    
    return msg;
}

// HttpClient implementation
HttpClient::HttpClient() {
}

HttpClient::~HttpClient() {
}

bool HttpClient::Get(const String& url, String& response) {
    // Parse the URL - simplified for this example
    int port = 80; // default HTTP port
    String host = url;
    String path = "/";

    // Find host and path
    if (url.StartsWith("http://")) {
        host = url.Mid(7);
    } else if (url.StartsWith("https://")) {
        host = url.Mid(8);
        port = 443; // default HTTPS port
    }

    int slashPos = host.Find('/');
    if (slashPos > 0) {
        path = host.Mid(slashPos);
        host = host.Mid(0, slashPos);
    }

    int portPos = host.Find(':');
    if (portPos > 0) {
        port = StrInt(host.Mid(portPos + 1));
        host = host.Mid(0, portPos);
    }

    // Create a TCP socket connection
    TcpSocket socket;
    if (!socket.Open()) {
        return false;
    }

    if (!socket.Connect(host, port)) {
        socket.Close();
        return false;
    }

    // Send HTTP GET request
    String request = "GET " + path + " HTTP/1.1\r\n";
    request += "Host: " + host + "\r\n";
    request += "Connection: close\r\n";
    
    // Add custom headers
    for (int i = 0; i < headers.GetCount(); i++) {
        request += headers.GetKey(i) + ": " + headers[i] + "\r\n";
    }
    
    request += "\r\n";

    if (!socket.Put(request)) {
        socket.Close();
        return false;
    }

    // Read response
    String responseData;
    String chunk;
    while (socket.Get(chunk, 0, 5000)) { // 5 second timeout
        responseData += chunk;
    }

    // Parse response
    int headerEnd = responseData.Find("\r\n\r\n");
    if (headerEnd > 0) {
        String headerData = responseData.Mid(0, headerEnd);
        String body = responseData.Mid(headerEnd + 4);

        // Parse headers
        Vector<String> headerLines = Split(headerData, "\r\n");
        if (!headerLines.IsEmpty()) {
            // Parse status line
            String statusLine = headerLines[0];
            Vector<String> parts = Split(statusLine, ' ');
            if (parts.GetCount() > 1) {
                lastStatusCode = StrInt(parts[1]);
            }
        }

        // Parse response headers
        responseHeaders.Clear();
        for (int i = 1; i < headerLines.GetCount(); i++) {
            String line = headerLines[i];
            int colonPos = line.Find(':');
            if (colonPos > 0) {
                String name = line.Mid(0, colonPos);
                String value = line.Mid(colonPos + 1).TrimBoth();
                responseHeaders.GetAdd(ToLower(Trim(name))) = Trim(value);
            }
        }

        response = body;
    } else {
        response = responseData;
    }

    socket.Close();
    return true;
}

bool HttpClient::Post(const String& url, const String& data, String& response) {
    // Parse the URL - simplified for this example
    int port = 80; // default HTTP port
    String host = url;
    String path = "/";

    // Find host and path
    if (url.StartsWith("http://")) {
        host = url.Mid(7);
    } else if (url.StartsWith("https://")) {
        host = url.Mid(8);
        port = 443; // default HTTPS port
    }

    int slashPos = host.Find('/');
    if (slashPos > 0) {
        path = host.Mid(slashPos);
        host = host.Mid(0, slashPos);
    }

    int portPos = host.Find(':');
    if (portPos > 0) {
        port = StrInt(host.Mid(portPos + 1));
        host = host.Mid(0, portPos);
    }

    // Create a TCP socket connection
    TcpSocket socket;
    if (!socket.Open()) {
        return false;
    }

    if (!socket.Connect(host, port)) {
        socket.Close();
        return false;
    }

    // Send HTTP POST request
    String request = "POST " + path + " HTTP/1.1\r\n";
    request += "Host: " + host + "\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + AsString(data.GetLength()) + "\r\n";
    request += "Connection: close\r\n";
    
    // Add custom headers
    for (int i = 0; i < headers.GetCount(); i++) {
        request += headers.GetKey(i) + ": " + headers[i] + "\r\n";
    }
    
    request += "\r\n";
    request += data;

    if (!socket.Put(request)) {
        socket.Close();
        return false;
    }

    // Read response
    String responseData;
    String chunk;
    while (socket.Get(chunk, 0, 5000)) { // 5 second timeout
        responseData += chunk;
    }

    // Parse response
    int headerEnd = responseData.Find("\r\n\r\n");
    if (headerEnd > 0) {
        String headerData = responseData.Mid(0, headerEnd);
        String body = responseData.Mid(headerEnd + 4);

        // Parse headers
        Vector<String> headerLines = Split(headerData, "\r\n");
        if (!headerLines.IsEmpty()) {
            // Parse status line
            String statusLine = headerLines[0];
            Vector<String> parts = Split(statusLine, ' ');
            if (parts.GetCount() > 1) {
                lastStatusCode = StrInt(parts[1]);
            }
        }

        // Parse response headers
        responseHeaders.Clear();
        for (int i = 1; i < headerLines.GetCount(); i++) {
            String line = headerLines[i];
            int colonPos = line.Find(':');
            if (colonPos > 0) {
                String name = line.Mid(0, colonPos);
                String value = line.Mid(colonPos + 1).TrimBoth();
                responseHeaders.GetAdd(ToLower(Trim(name))) = Trim(value);
            }
        }

        response = body;
    } else {
        response = responseData;
    }

    socket.Close();
    return true;
}

bool HttpClient::Put(const String& url, const String& data, String& response) {
    // Similar to POST but with PUT method
    return Post(url, data, response); // Simplified implementation
}

bool HttpClient::Delete(const String& url, String& response) {
    // Parse the URL - simplified for this example
    int port = 80; // default HTTP port
    String host = url;
    String path = "/";

    // Find host and path
    if (url.StartsWith("http://")) {
        host = url.Mid(7);
    } else if (url.StartsWith("https://")) {
        host = url.Mid(8);
        port = 443; // default HTTPS port
    }

    int slashPos = host.Find('/');
    if (slashPos > 0) {
        path = host.Mid(slashPos);
        host = host.Mid(0, slashPos);
    }

    int portPos = host.Find(':');
    if (portPos > 0) {
        port = StrInt(host.Mid(portPos + 1));
        host = host.Mid(0, portPos);
    }

    // Create a TCP socket connection
    TcpSocket socket;
    if (!socket.Open()) {
        return false;
    }

    if (!socket.Connect(host, port)) {
        socket.Close();
        return false;
    }

    // Send HTTP DELETE request
    String request = "DELETE " + path + " HTTP/1.1\r\n";
    request += "Host: " + host + "\r\n";
    request += "Connection: close\r\n";
    
    // Add custom headers
    for (int i = 0; i < headers.GetCount(); i++) {
        request += headers.GetKey(i) + ": " + headers[i] + "\r\n";
    }
    
    request += "\r\n";

    if (!socket.Put(request)) {
        socket.Close();
        return false;
    }

    // Read response
    String responseData;
    String chunk;
    while (socket.Get(chunk, 0, 5000)) { // 5 second timeout
        responseData += chunk;
    }

    // Parse response
    int headerEnd = responseData.Find("\r\n\r\n");
    if (headerEnd > 0) {
        String headerData = responseData.Mid(0, headerEnd);
        String body = responseData.Mid(headerEnd + 4);

        // Parse headers
        Vector<String> headerLines = Split(headerData, "\r\n");
        if (!headerLines.IsEmpty()) {
            // Parse status line
            String statusLine = headerLines[0];
            Vector<String> parts = Split(statusLine, ' ');
            if (parts.GetCount() > 1) {
                lastStatusCode = StrInt(parts[1]);
            }
        }

        // Parse response headers
        responseHeaders.Clear();
        for (int i = 1; i < headerLines.GetCount(); i++) {
            String line = headerLines[i];
            int colonPos = line.Find(':');
            if (colonPos > 0) {
                String name = line.Mid(0, colonPos);
                String value = line.Mid(colonPos + 1).TrimBoth();
                responseHeaders.GetAdd(ToLower(Trim(name))) = Trim(value);
            }
        }

        response = body;
    } else {
        response = responseData;
    }

    socket.Close();
    return true;
}

void HttpClient::SetHeader(const String& name, const String& value) {
    headers.GetAdd(ToLower(Trim(name))) = Trim(value);
}

String HttpClient::FormatHeaders() const {
    String headerStr;
    for (int i = 0; i < headers.GetCount(); i++) {
        headerStr += headers.GetKey(i) + ": " + headers[i] + "\r\n";
    }
    return headerStr;
}

// WebSocket implementation
WebSocket::WebSocket() {
}

WebSocket::~WebSocket() {
    Disconnect();
}

bool WebSocket::Initialize() {
    initialized = true;
    return true;
}

bool WebSocket::Connect(const String& url) {
    if (!initialized) {
        return false;
    }

    this->url = url;

    // Parse URL
    String host = url;
    String path = "/";

    if (url.StartsWith("ws://")) {
        host = url.Mid(5);
    } else if (url.StartsWith("wss://")) {
        host = url.Mid(6);
        // wss requires secure connection, which we're not implementing here
    }

    int slashPos = host.Find('/');
    if (slashPos > 0) {
        path = host.Mid(slashPos);
        host = host.Mid(0, slashPos);
    }

    int port = host.Find(':') > 0 ? StrInt(host.Mid(host.Find(':') + 1)) : 80;
    if (host.Find(':') > 0) {
        host = host.Mid(0, host.Find(':'));
    }

    if (!socket.Open()) {
        return false;
    }

    if (!socket.Connect(host, port)) {
        socket.Close();
        return false;
    }

    // Perform WebSocket handshake
    if (!Handshake(path)) {
        socket.Close();
        return false;
    }

    connected = true;
    return true;
}

bool WebSocket::Handshake(const String& path) {
    // Generate random key for handshake
    String key = "dGhlIHNhbXBsZSBub25jZQ=="; // In a real implementation, use a random value

    String request = "GET " + path + " HTTP/1.1\r\n";
    request += "Host: localhost\r\n"; // Should use actual host
    request += "Upgrade: websocket\r\n";
    request += "Connection: Upgrade\r\n";
    request += "Sec-WebSocket-Key: " + key + "\r\n";
    request += "Sec-WebSocket-Version: 13\r\n";
    request += "\r\n";

    if (!socket.Put(request)) {
        return false;
    }

    // Read response (simplified)
    String response;
    String chunk;
    while (socket.Get(chunk, 0, 5000)) { // 5 second timeout
        response += chunk;
        if (response.Find("\r\n\r\n") >= 0) break; // End of headers
    }

    // In a real implementation, we would validate the response
    return response.Find("101 Switching Protocols") >= 0;
}

bool WebSocket::SendMessage(const NetworkMessage& msg) {
    if (!connected) {
        return false;
    }

    String serialized = SerializeMessage(msg);
    String frame = CreateFrame(serialized);
    
    if (socket.Put(frame)) {
        bytesSent += frame.GetLength();
        messageCountSent++;
        return true;
    }
    return false;
}

bool WebSocket::ReceiveMessage(NetworkMessage& msg) {
    if (!connected) {
        return false;
    }

    String frame;
    if (socket.Get(frame)) {
        bytesReceived += frame.GetLength();
        messageCountReceived++;
        
        String data = ParseFrame(frame);
        msg = DeserializeMessage(data);
        return true;
    }
    return false;
}

bool WebSocket::Disconnect() {
    connected = false;
    if (socket.IsOpen()) {
        socket.Close();
    }
    return true;
}

ValueMap WebSocket::GetStats() const {
    ValueMap stats;
    stats.Add("bytes_sent", bytesSent);
    stats.Add("bytes_received", bytesReceived);
    stats.Add("messages_sent", messageCountSent);
    stats.Add("messages_received", messageCountReceived);
    stats.Add("connected", connected);
    return stats;
}

String WebSocket::CreateFrame(const String& data) {
    // Simplified WebSocket frame creation (doesn't handle large payloads)
    String frame;
    frame.Cat(0x81);  // FIN bit set, text frame
    if (data.GetLength() < 126) {
        frame.Cat((char)data.GetLength());  // Payload length
    } else if (data.GetLength() < 65536) {
        frame.Cat(126);  // Extended payload length
        frame.Cat((char)(data.GetLength() >> 8));
        frame.Cat((char)(data.GetLength() & 0xFF));
    } else {
        frame.Cat(127);  // Extended payload length (64-bit)
        for (int i = 7; i >= 0; i--) {
            frame.Cat((char)((data.GetLength() >> (i * 8)) & 0xFF));
        }
    }
    frame.Cat(data);
    return frame;
}

String WebSocket::ParseFrame(const String& frame) {
    // Simplified WebSocket frame parsing
    if (frame.GetLength() < 2) return "";

    // Check FIN and opcode (expect text frame)
    char firstByte = frame[0];
    bool fin = (firstByte & 0x80) != 0;
    int opcode = firstByte & 0x0F;

    if (!fin || opcode != 1) {  // Not a text frame or not finished
        return "";
    }

    // Check if masked (client should not send masked frames)
    bool masked = (frame[1] & 0x80) != 0;
    int payloadOffset = 2;
    int payloadLen = frame[1] & 0x7F;

    if (payloadLen == 126) {
        payloadOffset = 4;
        payloadLen = ((unsigned char)frame[2] << 8) | (unsigned char)frame[3];
    } else if (payloadLen == 127) {
        payloadOffset = 10;
        payloadLen = 0;
        for (int i = 0; i < 8; i++) {
            payloadLen = (payloadLen << 8) | (unsigned char)frame[2 + i];
        }
    }

    if (masked) {
        payloadOffset += 4;  // Skip masking key
        // Apply unmasking (simplified)
        String result;
        for (int i = 0; i < payloadLen; i++) {
            char unmasked = frame[payloadOffset + i] ^ frame[payloadOffset - 4 + (i % 4)];
            result.Cat(unmasked);
        }
        return result;
    } else {
        return frame.Mid(payloadOffset, payloadLen);
    }
}

String WebSocket::SerializeMessage(const NetworkMessage& msg) {
    JsonBuilder builder;
    builder("type", int(msg.type))
           ("sender_id", msg.senderId)
           ("target_id", msg.targetId)
           ("content", msg.content)
           ("timestamp", msg.timestamp.ToString());
    
    if (!msg.metadata.IsEmpty()) {
        builder("metadata", msg.metadata);
    }
    
    return builder.ToString();
}

NetworkMessage WebSocket::DeserializeMessage(const String& data) {
    NetworkMessage msg;
    
    try {
        JsonParser parser(data);
        JsonValue json = parser;
        
        if (json.IsObject()) {
            msg.type = NetworkMessageType(int(json("type", 0)));
            msg.senderId = json("sender_id", -1);
            msg.targetId = json("target_id", -1);
            msg.content = json("content", "");
            String timestampStr = json("timestamp", "");
            if (!timestampStr.IsEmpty()) {
                msg.timestamp = ScanTime(timestampStr);
            }
        }
    }
    catch (...) {
        // If parsing fails, return an empty message
    }
    
    return msg;
}

// NetworkManager implementation
NetworkManager::NetworkManager() {
}

NetworkManager::~NetworkManager() {
    interfaces.Clear();
    initialized = false;
}

bool NetworkManager::Initialize() {
    initialized = true;
    return true;
}

void NetworkManager::AddInterface(std::shared_ptr<NetworkInterface> interface) {
    if (interface && initialized) {
        interfaces.Add(interface);
    }
}

std::shared_ptr<NetworkInterface> NetworkManager::GetInterface(int index) {
    if (index >= 0 && index < interfaces.GetCount()) {
        return interfaces[index];
    }
    return nullptr;
}

void NetworkManager::Update() {
    // Check all interfaces for incoming messages
    for (auto& interface : interfaces) {
        NetworkMessage msg;
        if (interface && interface->IsConnected()) {
            while (interface->ReceiveMessage(msg)) {
                if (OnMessageReceived) {
                    OnMessageReceived(msg);
                }
            }
        }
    }
}

// GameNetworkUtils implementation
NetworkMessage GameNetworkUtils::SerializeGameState(const Value& gameState) {
    NetworkMessage msg(NetworkMessageType::GAME_STATE_UPDATE, AsJSON(gameState));
    return msg;
}

Value GameNetworkUtils::DeserializeGameState(const NetworkMessage& msg) {
    if (msg.type != NetworkMessageType::GAME_STATE_UPDATE) {
        return Value();
    }
    return FromJSON(msg.content);
}

NetworkMessage GameNetworkUtils::SerializePlayerInput(const Value& input) {
    NetworkMessage msg(NetworkMessageType::PLAYER_INPUT, AsJSON(input));
    return msg;
}

Value GameNetworkUtils::DeserializePlayerInput(const NetworkMessage& msg) {
    if (msg.type != NetworkMessageType::PLAYER_INPUT) {
        return Value();
    }
    return FromJSON(msg.content);
}

END_UPP_NAMESPACE