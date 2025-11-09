#pragma once

#include "Common.h"
#include "Order.h"
#include "Trade.h"
#include <map>
#include <list>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>

namespace MatchingEngine {

// Price level - maintains orders at a specific price
class PriceLevel {
public:
    explicit PriceLevel(Price price) : price_(price), totalQuantity_(0) {}

    void addOrder(OrderPtr order);
    void removeOrder(OrderId orderId);
    OrderPtr getOrder(OrderId orderId);
    
    Price getPrice() const { return price_; }
    Quantity getTotalQuantity() const { return totalQuantity_; }
    size_t getOrderCount() const { return orders_.size(); }
    bool isEmpty() const { return orders_.empty(); }
    
    const std::list<OrderPtr>& getOrders() const { return orders_; }
    std::list<OrderPtr>& getOrders() { return orders_; }

private:
    Price price_;
    Quantity totalQuantity_;
    std::list<OrderPtr> orders_;  // FIFO queue for price-time priority
    std::unordered_map<OrderId, std::list<OrderPtr>::iterator> orderMap_;  // Fast lookup
};

// Order book for a single symbol
class OrderBook {
public:
    explicit OrderBook(const std::string& symbol);

    // Order operations
    void addOrder(OrderPtr order);
    bool cancelOrder(OrderId orderId);
    bool modifyOrder(OrderId orderId, Price newPrice, Quantity newQuantity);
    OrderPtr getOrder(OrderId orderId);

    // Matching
    std::vector<Trade> matchOrder(OrderPtr order);

    // Market data
    Price getBestBid() const;
    Price getBestAsk() const;
    Quantity getBidQuantityAtLevel(Price price) const;
    Quantity getAskQuantityAtLevel(Price price) const;
    
    const std::string& getSymbol() const { return symbol_; }
    
    // Book depth
    std::vector<std::pair<Price, Quantity>> getBidDepth(size_t levels = 10) const;
    std::vector<std::pair<Price, Quantity>> getAskDepth(size_t levels = 10) const;

    // Display
    void printBook(size_t levels = 5) const;

private:
    std::string symbol_;
    
    // Buy orders (bids) - sorted descending (highest price first)
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    
    // Sell orders (asks) - sorted ascending (lowest price first)
    std::map<Price, PriceLevel, std::less<Price>> asks_;
    
    // Fast order lookup
    std::unordered_map<OrderId, OrderPtr> orderMap_;
    
    // Thread safety
    mutable std::mutex mutex_;

    // Helper methods
    std::vector<Trade> matchMarketOrder(OrderPtr order);
    std::vector<Trade> matchLimitOrder(OrderPtr order);
    std::vector<Trade> matchIOCOrder(OrderPtr order);
    std::vector<Trade> matchFOKOrder(OrderPtr order);
    
    std::vector<Trade> executeMatches(OrderPtr order, 
                                      std::map<Price, PriceLevel, std::less<Price>>& levels,
                                      bool fillEntireOrder = false);
    std::vector<Trade> executeMatches(OrderPtr order, 
                                      std::map<Price, PriceLevel, std::greater<Price>>& levels,
                                      bool fillEntireOrder = false);
    
    bool canFillEntireOrder(OrderPtr order, 
                           const std::map<Price, PriceLevel, std::less<Price>>& levels) const;
    bool canFillEntireOrder(OrderPtr order, 
                           const std::map<Price, PriceLevel, std::greater<Price>>& levels) const;
};

} // namespace MatchingEngine

