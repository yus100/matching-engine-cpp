#include "Server.h"
#include <iostream>
#include <cstring>

namespace MatchingEngine {

Server::Server(uint16_t port)
    : port_(port)
    , serverSocket_(INVALID_SOCKET)
    , running_(false)
    , activeConnections_(0) {
    
    engine_ = std::make_unique<MatchingEngineCore>();
    
    // Set up callbacks
    engine_->setOrderCallback([](const OrderPtr& order) {
        std::cout << "[ENGINE] Order update: " << order->toString() << std::endl;
    });
    
    engine_->setTradeCallback([](const Trade& trade) {
        std::cout << "[ENGINE] Trade executed: " << trade.toString() << std::endl;
    });
    
    initializeSocket();
}

Server::~Server() {
    stop();
    cleanupSocket();
}

void Server::initializeSocket() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
    }
#endif
}

void Server::cleanupSocket() {
#ifdef _WIN32
    WSACleanup();
#endif
}

bool Server::start() {
    if (running_) {
        std::cerr << "Server is already running" << std::endl;
        return false;
    }
    
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket_ == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // Set socket options
    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    // Bind socket
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);
    
    if (bind(serverSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind socket to port " << port_ << std::endl;
        closesocket(serverSocket_);
        return false;
    }
    
    // Listen
    if (listen(serverSocket_, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Failed to listen on socket" << std::endl;
        closesocket(serverSocket_);
        return false;
    }
    
    running_ = true;
    
    // Start accept thread
    acceptThread_ = std::thread(&Server::acceptClients, this);
    
    std::cout << "Server started on port " << port_ << std::endl;
    return true;
}

void Server::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // Close server socket to unblock accept
    if (serverSocket_ != INVALID_SOCKET) {
        closesocket(serverSocket_);
        serverSocket_ = INVALID_SOCKET;
    }
    
    // Wait for accept thread
    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
    
    // Wait for client threads
    for (auto& thread : clientThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    clientThreads_.clear();
    
    std::cout << "Server stopped" << std::endl;
}

void Server::acceptClients() {
    while (running_) {
        sockaddr_in clientAddr{};
#ifdef _WIN32
        int clientAddrLen = sizeof(clientAddr);
#else
        socklen_t clientAddrLen = sizeof(clientAddr);
#endif
        
        SocketType clientSocket = accept(serverSocket_, (sockaddr*)&clientAddr, &clientAddrLen);
        
        if (clientSocket == INVALID_SOCKET) {
            if (running_) {
                std::cerr << "Failed to accept client connection" << std::endl;
            }
            continue;
        }
        
        activeConnections_++;
        std::cout << "Client connected. Active connections: " << activeConnections_ << std::endl;
        
        // Handle client in new thread
        clientThreads_.emplace_back(&Server::handleClient, this, clientSocket);
    }
}

void Server::handleClient(SocketType clientSocket) {
    std::vector<char> buffer(MAX_MESSAGE_SIZE);
    
    while (running_) {
        // Read message header
        MessageHeader header;
        if (!receiveMessage(clientSocket, &header, sizeof(MessageHeader))) {
            break;
        }
        
        // Handle message based on type
        switch (header.type) {
            case MessageType::NEW_ORDER: {
                NewOrderMessage msg;
                if (receiveMessage(clientSocket, &msg, sizeof(NewOrderMessage))) {
                    handleNewOrder(clientSocket, msg);
                }
                break;
            }
            
            case MessageType::CANCEL_ORDER: {
                CancelOrderMessage msg;
                if (receiveMessage(clientSocket, &msg, sizeof(CancelOrderMessage))) {
                    handleCancelOrder(clientSocket, msg);
                }
                break;
            }
            
            case MessageType::MODIFY_ORDER: {
                ModifyOrderMessage msg;
                if (receiveMessage(clientSocket, &msg, sizeof(ModifyOrderMessage))) {
                    handleModifyOrder(clientSocket, msg);
                }
                break;
            }
            
            case MessageType::HEARTBEAT: {
                HeartbeatMessage msg;
                if (receiveMessage(clientSocket, &msg, sizeof(HeartbeatMessage))) {
                    // Echo heartbeat back
                    sendMessage(clientSocket, &msg, sizeof(HeartbeatMessage));
                }
                break;
            }
            
            default:
                std::cerr << "Unknown message type received" << std::endl;
                break;
        }
    }
    
    closesocket(clientSocket);
    activeConnections_--;
    std::cout << "Client disconnected. Active connections: " << activeConnections_ << std::endl;
}

void Server::handleNewOrder(SocketType clientSocket, const NewOrderMessage& msg) {
    std::cout << "[SERVER] New order: " << msg.getSymbol() 
              << " " << sideToString(msg.side)
              << " " << msg.quantity << " @ " << priceToDouble(msg.price) << std::endl;
    
    // Submit to engine
    OrderId orderId = engine_->submitOrder(
        msg.getSymbol(),
        msg.side,
        msg.orderType,
        msg.price,
        msg.quantity,
        msg.getClientId(),
        msg.stopPrice
    );
    
    // Send acknowledgment
    OrderAckMessage ack;
    ack.clientOrderId = msg.clientOrderId;
    ack.orderId = orderId;
    ack.status = OrderStatus::PENDING;
    ack.setMessage("Order accepted");
    
    sendMessage(clientSocket, &ack, sizeof(OrderAckMessage));
    
    // Get final order status and send execution report if filled
    auto order = engine_->getOrder(orderId);
    if (order && order->getStatus() != OrderStatus::PENDING) {
        ExecutionReportMessage exec;
        exec.orderId = orderId;
        exec.setSymbol(order->getSymbol());
        exec.side = order->getSide();
        exec.executionPrice = order->getPrice();
        exec.executionQuantity = order->getFilledQuantity();
        exec.remainingQuantity = order->getRemainingQuantity();
        exec.status = order->getStatus();
        
        sendMessage(clientSocket, &exec, sizeof(ExecutionReportMessage));
    }
}

void Server::handleCancelOrder(SocketType clientSocket, const CancelOrderMessage& msg) {
    std::cout << "[SERVER] Cancel order: " << msg.orderId << std::endl;
    
    bool success = engine_->cancelOrder(msg.orderId);
    
    OrderAckMessage ack;
    ack.orderId = msg.orderId;
    
    if (success) {
        ack.status = OrderStatus::CANCELLED;
        ack.setMessage("Order cancelled");
    } else {
        ack.status = OrderStatus::REJECTED;
        ack.setMessage("Order not found");
    }
    
    sendMessage(clientSocket, &ack, sizeof(OrderAckMessage));
}

void Server::handleModifyOrder(SocketType clientSocket, const ModifyOrderMessage& msg) {
    std::cout << "[SERVER] Modify order: " << msg.orderId 
              << " new price: " << priceToDouble(msg.newPrice)
              << " new qty: " << msg.newQuantity << std::endl;
    
    bool success = engine_->modifyOrder(msg.orderId, msg.newPrice, msg.newQuantity);
    
    OrderAckMessage ack;
    ack.orderId = msg.orderId;
    
    if (success) {
        ack.status = OrderStatus::PENDING;
        ack.setMessage("Order modified");
    } else {
        ack.status = OrderStatus::REJECTED;
        ack.setMessage("Failed to modify order");
    }
    
    sendMessage(clientSocket, &ack, sizeof(OrderAckMessage));
}

bool Server::sendMessage(SocketType socket, const void* data, size_t length) {
    size_t totalSent = 0;
    const char* buffer = static_cast<const char*>(data);
    
    while (totalSent < length) {
        int sent = send(socket, buffer + totalSent, static_cast<int>(length - totalSent), 0);
        if (sent == SOCKET_ERROR) {
            return false;
        }
        totalSent += sent;
    }
    
    return true;
}

bool Server::receiveMessage(SocketType socket, void* buffer, size_t length) {
    size_t totalReceived = 0;
    char* buf = static_cast<char*>(buffer);
    
    while (totalReceived < length) {
        int received = recv(socket, buf + totalReceived, static_cast<int>(length - totalReceived), 0);
        if (received <= 0) {
            return false;
        }
        totalReceived += received;
    }
    
    return true;
}

size_t Server::getTotalOrders() const {
    return engine_->getTotalOrders();
}

size_t Server::getTotalTrades() const {
    return engine_->getTotalTrades();
}

} // namespace MatchingEngine

