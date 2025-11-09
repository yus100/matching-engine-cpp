#pragma once

#include "Common.h"
#include "Order.h"
#include "Trade.h"
#include "OrderBook.h"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>

namespace MatchingEngine {

// Callback types for notifications
using OrderCallback = std::function<void(const OrderPtr&)>;
using TradeCallback = std::function<void(const Trade&)>;

class MatchingEngineCore {
public:
    MatchingEngineCore();
    ~MatchingEngineCore() = default;

    // Order operations
    OrderId submitOrder(const std::string& symbol,
                       Side side,
                       OrderType type,
                       Price price,
                       Quantity quantity,
                       const std::string& clientId = "",
                       Price stopPrice = 0);
    
    bool cancelOrder(OrderId orderId);
    bool modifyOrder(OrderId orderId, Price newPrice, Quantity newQuantity);
    OrderPtr getOrder(OrderId orderId);

    // Market data
    Price getBestBid(const std::string& symbol);
    Price getBestAsk(const std::string& symbol);
    std::vector<std::pair<Price, Quantity>> getBidDepth(const std::string& symbol, size_t levels = 10);
    std::vector<std::pair<Price, Quantity>> getAskDepth(const std::string& symbol, size_t levels = 10);

    // Display
    void printOrderBook(const std::string& symbol, size_t levels = 5);

    // Callbacks
    void setOrderCallback(OrderCallback callback) { orderCallback_ = callback; }
    void setTradeCallback(TradeCallback callback) { tradeCallback_ = callback; }

    // Statistics
    size_t getTotalOrders() const { return totalOrders_; }
    size_t getTotalTrades() const { return totalTrades_; }

private:
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> orderBooks_;
    std::unordered_map<OrderId, std::string> orderToSymbol_;  // Track which symbol each order belongs to
    
    std::atomic<OrderId> nextOrderId_;
    std::atomic<size_t> totalOrders_;
    std::atomic<size_t> totalTrades_;
    
    mutable std::mutex mutex_;

    // Callbacks
    OrderCallback orderCallback_;
    TradeCallback tradeCallback_;

    // Helper methods
    OrderBook* getOrCreateOrderBook(const std::string& symbol);
    void notifyOrder(const OrderPtr& order);
    void notifyTrade(const Trade& trade);
};

} // namespace MatchingEngine

