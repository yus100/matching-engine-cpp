#pragma once

#include "Common.h"
#include "MatchingEngine.h"
#include "Message.h"
#include <memory>
#include <thread>
#include <atomic>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET SocketType;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int SocketType;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

namespace MatchingEngine {

class Server {
public:
    explicit Server(uint16_t port = SERVER_PORT);
    ~Server();

    // Server lifecycle
    bool start();
    void stop();
    bool isRunning() const { return running_; }

    // Statistics
    size_t getActiveConnections() const { return activeConnections_; }
    size_t getTotalOrders() const;
    size_t getTotalTrades() const;

private:
    uint16_t port_;
    SocketType serverSocket_;
    std::atomic<bool> running_;
    std::atomic<size_t> activeConnections_;
    
    std::unique_ptr<MatchingEngineCore> engine_;
    std::thread acceptThread_;
    std::vector<std::thread> clientThreads_;

    // Network operations
    void acceptClients();
    void handleClient(SocketType clientSocket);
    
    // Message handlers
    void handleNewOrder(SocketType clientSocket, const NewOrderMessage& msg);
    void handleCancelOrder(SocketType clientSocket, const CancelOrderMessage& msg);
    void handleModifyOrder(SocketType clientSocket, const ModifyOrderMessage& msg);
    
    // Utilities
    bool sendMessage(SocketType socket, const void* data, size_t length);
    bool receiveMessage(SocketType socket, void* buffer, size_t length);
    void initializeSocket();
    void cleanupSocket();
};

} // namespace MatchingEngine

