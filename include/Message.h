#pragma once

#include "Common.h"
#include <string>
#include <cstring>
#include <vector>

namespace MatchingEngine {

// Base message structure
struct MessageHeader {
    MessageType type;
    uint32_t length;  // Total message length including header
    uint64_t timestamp;
    
    MessageHeader() : type(MessageType::HEARTBEAT), length(0), timestamp(0) {}
    MessageHeader(MessageType t, uint32_t len) 
        : type(t), length(len), timestamp(0) {}
};

// New order request
struct NewOrderMessage {
    MessageHeader header;
    OrderId clientOrderId;  // Client-generated ID
    char symbol[16];
    Side side;
    OrderType orderType;
    Price price;
    Quantity quantity;
    Price stopPrice;
    char clientId[32];
    
    NewOrderMessage() {
        header.type = MessageType::NEW_ORDER;
        header.length = sizeof(NewOrderMessage);
        clientOrderId = 0;
        std::memset(symbol, 0, sizeof(symbol));
        side = Side::BUY;
        orderType = OrderType::LIMIT;
        price = 0;
        quantity = 0;
        stopPrice = 0;
        std::memset(clientId, 0, sizeof(clientId));
    }
    
    void setSymbol(const std::string& s) {
        std::strncpy(symbol, s.c_str(), sizeof(symbol) - 1);
    }
    
    void setClientId(const std::string& id) {
        std::strncpy(clientId, id.c_str(), sizeof(clientId) - 1);
    }
    
    std::string getSymbol() const {
        return std::string(symbol);
    }
    
    std::string getClientId() const {
        return std::string(clientId);
    }
};

// Cancel order request
struct CancelOrderMessage {
    MessageHeader header;
    OrderId orderId;
    char clientId[32];
    
    CancelOrderMessage() {
        header.type = MessageType::CANCEL_ORDER;
        header.length = sizeof(CancelOrderMessage);
        orderId = 0;
        std::memset(clientId, 0, sizeof(clientId));
    }
    
    void setClientId(const std::string& id) {
        std::strncpy(clientId, id.c_str(), sizeof(clientId) - 1);
    }
    
    std::string getClientId() const {
        return std::string(clientId);
    }
};

// Modify order request
struct ModifyOrderMessage {
    MessageHeader header;
    OrderId orderId;
    Price newPrice;
    Quantity newQuantity;
    char clientId[32];
    
    ModifyOrderMessage() {
        header.type = MessageType::MODIFY_ORDER;
        header.length = sizeof(ModifyOrderMessage);
        orderId = 0;
        newPrice = 0;
        newQuantity = 0;
        std::memset(clientId, 0, sizeof(clientId));
    }
    
    void setClientId(const std::string& id) {
        std::strncpy(clientId, id.c_str(), sizeof(clientId) - 1);
    }
    
    std::string getClientId() const {
        return std::string(clientId);
    }
};

// Order acknowledgment
struct OrderAckMessage {
    MessageHeader header;
    OrderId clientOrderId;
    OrderId orderId;  // Server-assigned ID
    OrderStatus status;
    char message[128];
    
    OrderAckMessage() {
        header.type = MessageType::ORDER_ACK;
        header.length = sizeof(OrderAckMessage);
        clientOrderId = 0;
        orderId = 0;
        status = OrderStatus::PENDING;
        std::memset(message, 0, sizeof(message));
    }
    
    void setMessage(const std::string& msg) {
        std::strncpy(message, msg.c_str(), sizeof(message) - 1);
    }
    
    std::string getMessage() const {
        return std::string(message);
    }
};

// Order rejection
struct OrderRejectMessage {
    MessageHeader header;
    OrderId clientOrderId;
    char reason[256];
    
    OrderRejectMessage() {
        header.type = MessageType::ORDER_REJECT;
        header.length = sizeof(OrderRejectMessage);
        clientOrderId = 0;
        std::memset(reason, 0, sizeof(reason));
    }
    
    void setReason(const std::string& r) {
        std::strncpy(reason, r.c_str(), sizeof(reason) - 1);
    }
    
    std::string getReason() const {
        return std::string(reason);
    }
};

// Execution report (trade notification)
struct ExecutionReportMessage {
    MessageHeader header;
    OrderId orderId;
    char symbol[16];
    Side side;
    Price executionPrice;
    Quantity executionQuantity;
    Quantity remainingQuantity;
    OrderStatus status;
    uint64_t tradeId;
    
    ExecutionReportMessage() {
        header.type = MessageType::EXECUTION_REPORT;
        header.length = sizeof(ExecutionReportMessage);
        orderId = 0;
        std::memset(symbol, 0, sizeof(symbol));
        side = Side::BUY;
        executionPrice = 0;
        executionQuantity = 0;
        remainingQuantity = 0;
        status = OrderStatus::PENDING;
        tradeId = 0;
    }
    
    void setSymbol(const std::string& s) {
        std::strncpy(symbol, s.c_str(), sizeof(symbol) - 1);
    }
    
    std::string getSymbol() const {
        return std::string(symbol);
    }
};

// Market data update
struct MarketDataMessage {
    MessageHeader header;
    char symbol[16];
    Price bestBid;
    Price bestAsk;
    Quantity bidQuantity;
    Quantity askQuantity;
    
    MarketDataMessage() {
        header.type = MessageType::MARKET_DATA;
        header.length = sizeof(MarketDataMessage);
        std::memset(symbol, 0, sizeof(symbol));
        bestBid = 0;
        bestAsk = 0;
        bidQuantity = 0;
        askQuantity = 0;
    }
    
    void setSymbol(const std::string& s) {
        std::strncpy(symbol, s.c_str(), sizeof(symbol) - 1);
    }
    
    std::string getSymbol() const {
        return std::string(symbol);
    }
};

// Heartbeat message
struct HeartbeatMessage {
    MessageHeader header;
    uint64_t sequenceNumber;
    
    HeartbeatMessage() {
        header.type = MessageType::HEARTBEAT;
        header.length = sizeof(HeartbeatMessage);
        sequenceNumber = 0;
    }
};

// Helper functions for serialization
class MessageSerializer {
public:
    // Serialize message to bytes
    template<typename T>
    static std::vector<char> serialize(const T& message) {
        std::vector<char> buffer(sizeof(T));
        std::memcpy(buffer.data(), &message, sizeof(T));
        return buffer;
    }
    
    // Deserialize bytes to message
    template<typename T>
    static T deserialize(const std::vector<char>& buffer) {
        T message;
        if (buffer.size() >= sizeof(T)) {
            std::memcpy(&message, buffer.data(), sizeof(T));
        }
        return message;
    }
    
    // Read message header
    static MessageHeader readHeader(const std::vector<char>& buffer) {
        MessageHeader header;
        if (buffer.size() >= sizeof(MessageHeader)) {
            std::memcpy(&header, buffer.data(), sizeof(MessageHeader));
        }
        return header;
    }
};

} // namespace MatchingEngine

