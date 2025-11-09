#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace MatchingEngine {

// Type aliases for clarity and easy modification
using OrderId = uint64_t;
using Price = int64_t;  // Using fixed-point arithmetic (price * 10000 for 4 decimal places)
using Quantity = uint64_t;
using Timestamp = std::chrono::time_point<std::chrono::high_resolution_clock>;

// Order side
enum class Side {
    BUY,
    SELL
};

// Order types
enum class OrderType {
    MARKET,      // Execute immediately at best available price
    LIMIT,       // Execute at specified price or better
    STOP_LOSS,   // Becomes market order when stop price is reached
    STOP_LIMIT,  // Becomes limit order when stop price is reached
    IOC,         // Immediate or Cancel - execute immediately, cancel unfilled portion
    FOK          // Fill or Kill - execute completely or cancel entirely
};

// Order status
enum class OrderStatus {
    PENDING,
    PARTIAL_FILL,
    FILLED,
    CANCELLED,
    REJECTED
};

// Message types for client-server communication
enum class MessageType {
    NEW_ORDER,
    CANCEL_ORDER,
    MODIFY_ORDER,
    ORDER_ACK,
    ORDER_REJECT,
    EXECUTION_REPORT,
    MARKET_DATA,
    HEARTBEAT
};

// Constants
constexpr uint32_t MAX_ORDERS_PER_LEVEL = 10000;
constexpr uint32_t MAX_PRICE_LEVELS = 100000;
constexpr uint32_t SERVER_PORT = 8888;
constexpr uint32_t MAX_MESSAGE_SIZE = 4096;

// Utility functions
inline std::string sideToString(Side side) {
    return side == Side::BUY ? "BUY" : "SELL";
}

inline std::string orderTypeToString(OrderType type) {
    switch (type) {
        case OrderType::MARKET: return "MARKET";
        case OrderType::LIMIT: return "LIMIT";
        case OrderType::STOP_LOSS: return "STOP_LOSS";
        case OrderType::STOP_LIMIT: return "STOP_LIMIT";
        case OrderType::IOC: return "IOC";
        case OrderType::FOK: return "FOK";
        default: return "UNKNOWN";
    }
}

inline std::string orderStatusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::PENDING: return "PENDING";
        case OrderStatus::PARTIAL_FILL: return "PARTIAL_FILL";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::CANCELLED: return "CANCELLED";
        case OrderStatus::REJECTED: return "REJECTED";
        default: return "UNKNOWN";
    }
}

// Helper to get current timestamp
inline Timestamp getCurrentTimestamp() {
    return std::chrono::high_resolution_clock::now();
}

// Convert price to readable format (divide by 10000)
inline double priceToDouble(Price price) {
    return static_cast<double>(price) / 10000.0;
}

// Convert double to fixed-point price
inline Price doubleToPrice(double price) {
    return static_cast<Price>(price * 10000.0);
}

} // namespace MatchingEngine

