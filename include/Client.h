#pragma once

#include "Common.h"
#include "Message.h"
#include <string>
#include <atomic>
#include <thread>
#include <functional>
#include <mutex>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
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

// Response callbacks
using OrderAckCallback = std::function<void(const OrderAckMessage&)>;
using OrderRejectCallback = std::function<void(const OrderRejectMessage&)>;
using ExecutionReportCallback = std::function<void(const ExecutionReportMessage&)>;
using MarketDataCallback = std::function<void(const MarketDataMessage&)>;

class Client {
public:
    Client(const std::string& serverHost = "127.0.0.1", uint16_t serverPort = SERVER_PORT);
    ~Client();

    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const { return connected_; }

    // Order operations
    OrderId submitOrder(const std::string& symbol,
                       Side side,
                       OrderType type,
                       Price price,
                       Quantity quantity,
                       Price stopPrice = 0);
    
    bool cancelOrder(OrderId orderId);
    bool modifyOrder(OrderId orderId, Price newPrice, Quantity newQuantity);

    // Callbacks
    void setOrderAckCallback(OrderAckCallback callback) { orderAckCallback_ = callback; }
    void setOrderRejectCallback(OrderRejectCallback callback) { orderRejectCallback_ = callback; }
    void setExecutionReportCallback(ExecutionReportCallback callback) { executionReportCallback_ = callback; }
    void setMarketDataCallback(MarketDataCallback callback) { marketDataCallback_ = callback; }

    // Client ID
    void setClientId(const std::string& clientId) { clientId_ = clientId; }
    const std::string& getClientId() const { return clientId_; }

private:
    std::string serverHost_;
    uint16_t serverPort_;
    SocketType socket_;
    std::atomic<bool> connected_;
    std::atomic<OrderId> nextClientOrderId_;
    std::string clientId_;
    
    std::thread receiveThread_;
    
    // Callbacks
    OrderAckCallback orderAckCallback_;
    OrderRejectCallback orderRejectCallback_;
    ExecutionReportCallback executionReportCallback_;
    MarketDataCallback marketDataCallback_;
    
    std::mutex sendMutex_;

    // Network operations
    void receiveMessages();
    bool sendMessage(const void* data, size_t length);
    bool receiveMessage(void* buffer, size_t length);
    
    // Message handlers
    void handleOrderAck(const OrderAckMessage& msg);
    void handleOrderReject(const OrderRejectMessage& msg);
    void handleExecutionReport(const ExecutionReportMessage& msg);
    void handleMarketData(const MarketDataMessage& msg);
    
    void initializeSocket();
    void cleanupSocket();
};

} // namespace MatchingEngine

