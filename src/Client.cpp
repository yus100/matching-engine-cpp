#include "Client.h"
#include <iostream>
#include <cstring>

namespace MatchingEngine {

Client::Client(const std::string& serverHost, uint16_t serverPort)
    : serverHost_(serverHost)
    , serverPort_(serverPort)
    , socket_(INVALID_SOCKET)
    , connected_(false)
    , nextClientOrderId_(1)
    , clientId_("Client") {
    
    initializeSocket();
}

Client::~Client() {
    disconnect();
    cleanupSocket();
}

void Client::initializeSocket() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
    }
#endif
}

void Client::cleanupSocket() {
#ifdef _WIN32
    WSACleanup();
#endif
}

bool Client::connect() {
    if (connected_) {
        std::cerr << "Already connected to server" << std::endl;
        return false;
    }
    
    // Create socket
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // Connect to server
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort_);
    
#ifdef _WIN32
    serverAddr.sin_addr.s_addr = inet_addr(serverHost_.c_str());
#else
    inet_pton(AF_INET, serverHost_.c_str(), &serverAddr.sin_addr);
#endif
    
    if (::connect(socket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to server " << serverHost_ << ":" << serverPort_ << std::endl;
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
        return false;
    }
    
    connected_ = true;
    
    // Start receive thread
    receiveThread_ = std::thread(&Client::receiveMessages, this);
    
    std::cout << "Connected to server " << serverHost_ << ":" << serverPort_ << std::endl;
    return true;
}

void Client::disconnect() {
    if (!connected_) {
        return;
    }
    
    connected_ = false;
    
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
    
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }
    
    std::cout << "Disconnected from server" << std::endl;
}

OrderId Client::submitOrder(
    const std::string& symbol,
    Side side,
    OrderType type,
    Price price,
    Quantity quantity,
    Price stopPrice) {
    
    if (!connected_) {
        std::cerr << "Not connected to server" << std::endl;
        return 0;
    }
    
    OrderId clientOrderId = nextClientOrderId_++;
    
    NewOrderMessage msg;
    msg.clientOrderId = clientOrderId;
    msg.setSymbol(symbol);
    msg.side = side;
    msg.orderType = type;
    msg.price = price;
    msg.quantity = quantity;
    msg.stopPrice = stopPrice;
    msg.setClientId(clientId_);
    
    std::lock_guard<std::mutex> lock(sendMutex_);
    if (!sendMessage(&msg, sizeof(NewOrderMessage))) {
        std::cerr << "Failed to send order" << std::endl;
        return 0;
    }
    
    std::cout << "[CLIENT] Order sent: " << symbol << " " << sideToString(side)
              << " " << quantity << " @ " << priceToDouble(price) << std::endl;
    
    return clientOrderId;
}

bool Client::cancelOrder(OrderId orderId) {
    if (!connected_) {
        std::cerr << "Not connected to server" << std::endl;
        return false;
    }
    
    CancelOrderMessage msg;
    msg.orderId = orderId;
    msg.setClientId(clientId_);
    
    std::lock_guard<std::mutex> lock(sendMutex_);
    if (!sendMessage(&msg, sizeof(CancelOrderMessage))) {
        std::cerr << "Failed to send cancel order" << std::endl;
        return false;
    }
    
    std::cout << "[CLIENT] Cancel order sent: " << orderId << std::endl;
    return true;
}

bool Client::modifyOrder(OrderId orderId, Price newPrice, Quantity newQuantity) {
    if (!connected_) {
        std::cerr << "Not connected to server" << std::endl;
        return false;
    }
    
    ModifyOrderMessage msg;
    msg.orderId = orderId;
    msg.newPrice = newPrice;
    msg.newQuantity = newQuantity;
    msg.setClientId(clientId_);
    
    std::lock_guard<std::mutex> lock(sendMutex_);
    if (!sendMessage(&msg, sizeof(ModifyOrderMessage))) {
        std::cerr << "Failed to send modify order" << std::endl;
        return false;
    }
    
    std::cout << "[CLIENT] Modify order sent: " << orderId 
              << " new price: " << priceToDouble(newPrice)
              << " new qty: " << newQuantity << std::endl;
    return true;
}

void Client::receiveMessages() {
    std::vector<char> buffer(MAX_MESSAGE_SIZE);
    
    while (connected_) {
        // Read message header
        MessageHeader header;
        if (!receiveMessage(&header, sizeof(MessageHeader))) {
            if (connected_) {
                std::cerr << "Connection lost" << std::endl;
                connected_ = false;
            }
            break;
        }
        
        // Handle message based on type
        switch (header.type) {
            case MessageType::ORDER_ACK: {
                OrderAckMessage msg;
                if (receiveMessage(&msg, sizeof(OrderAckMessage))) {
                    handleOrderAck(msg);
                }
                break;
            }
            
            case MessageType::ORDER_REJECT: {
                OrderRejectMessage msg;
                if (receiveMessage(&msg, sizeof(OrderRejectMessage))) {
                    handleOrderReject(msg);
                }
                break;
            }
            
            case MessageType::EXECUTION_REPORT: {
                ExecutionReportMessage msg;
                if (receiveMessage(&msg, sizeof(ExecutionReportMessage))) {
                    handleExecutionReport(msg);
                }
                break;
            }
            
            case MessageType::MARKET_DATA: {
                MarketDataMessage msg;
                if (receiveMessage(&msg, sizeof(MarketDataMessage))) {
                    handleMarketData(msg);
                }
                break;
            }
            
            case MessageType::HEARTBEAT: {
                // Heartbeat received, ignore or handle
                break;
            }
            
            default:
                std::cerr << "Unknown message type received from server" << std::endl;
                break;
        }
    }
}

void Client::handleOrderAck(const OrderAckMessage& msg) {
    std::cout << "[CLIENT] Order ACK: Client Order " << msg.clientOrderId 
              << " -> Server Order " << msg.orderId 
              << " Status: " << orderStatusToString(msg.status)
              << " Message: " << msg.getMessage() << std::endl;
    
    if (orderAckCallback_) {
        orderAckCallback_(msg);
    }
}

void Client::handleOrderReject(const OrderRejectMessage& msg) {
    std::cout << "[CLIENT] Order REJECT: Client Order " << msg.clientOrderId 
              << " Reason: " << msg.getReason() << std::endl;
    
    if (orderRejectCallback_) {
        orderRejectCallback_(msg);
    }
}

void Client::handleExecutionReport(const ExecutionReportMessage& msg) {
    std::cout << "[CLIENT] Execution Report: Order " << msg.orderId
              << " Symbol: " << msg.getSymbol()
              << " Side: " << sideToString(msg.side)
              << " Executed: " << msg.executionQuantity << " @ " << priceToDouble(msg.executionPrice)
              << " Remaining: " << msg.remainingQuantity
              << " Status: " << orderStatusToString(msg.status) << std::endl;
    
    if (executionReportCallback_) {
        executionReportCallback_(msg);
    }
}

void Client::handleMarketData(const MarketDataMessage& msg) {
    std::cout << "[CLIENT] Market Data: " << msg.getSymbol()
              << " Bid: " << priceToDouble(msg.bestBid) << " x " << msg.bidQuantity
              << " Ask: " << priceToDouble(msg.bestAsk) << " x " << msg.askQuantity << std::endl;
    
    if (marketDataCallback_) {
        marketDataCallback_(msg);
    }
}

bool Client::sendMessage(const void* data, size_t length) {
    size_t totalSent = 0;
    const char* buffer = static_cast<const char*>(data);
    
    while (totalSent < length) {
        int sent = send(socket_, buffer + totalSent, static_cast<int>(length - totalSent), 0);
        if (sent == SOCKET_ERROR) {
            return false;
        }
        totalSent += sent;
    }
    
    return true;
}

bool Client::receiveMessage(void* buffer, size_t length) {
    size_t totalReceived = 0;
    char* buf = static_cast<char*>(buffer);
    
    while (totalReceived < length) {
        int received = recv(socket_, buf + totalReceived, static_cast<int>(length - totalReceived), 0);
        if (received <= 0) {
            return false;
        }
        totalReceived += received;
    }
    
    return true;
}

} // namespace MatchingEngine

