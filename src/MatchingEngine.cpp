#include "MatchingEngine.h"
#include <iostream>

namespace MatchingEngine {

MatchingEngineCore::MatchingEngineCore() 
    : nextOrderId_(1)
    , totalOrders_(0)
    , totalTrades_(0) {
}

OrderId MatchingEngineCore::submitOrder(
    const std::string& symbol,
    Side side,
    OrderType type,
    Price price,
    Quantity quantity,
    const std::string& clientId,
    Price stopPrice) {
    
    // Generate order ID
    OrderId orderId = nextOrderId_++;
    totalOrders_++;
    
    // Create order
    auto order = std::make_shared<Order>(orderId, symbol, side, type, price, quantity, stopPrice);
    order->setClientId(clientId);
    
    // Get or create order book
    OrderBook* book = getOrCreateOrderBook(symbol);
    
    // Track order to symbol mapping
    {
        std::lock_guard<std::mutex> lock(mutex_);
        orderToSymbol_[orderId] = symbol;
    }
    
    notifyOrder(order);
    
    // Match order
    std::vector<Trade> trades = book->matchOrder(order);
    
    // Notify trades
    for (const auto& trade : trades) {
        totalTrades_++;
        notifyTrade(trade);
    }
    
    // Notify final order status
    notifyOrder(order);
    
    return orderId;
}

bool MatchingEngineCore::cancelOrder(OrderId orderId) {
    std::string symbol;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = orderToSymbol_.find(orderId);
        if (it == orderToSymbol_.end()) {
            return false;
        }
        symbol = it->second;
    }
    
    auto bookIt = orderBooks_.find(symbol);
    if (bookIt == orderBooks_.end()) {
        return false;
    }
    
    bool success = bookIt->second->cancelOrder(orderId);
    
    if (success) {
        std::lock_guard<std::mutex> lock(mutex_);
        orderToSymbol_.erase(orderId);
    }
    
    return success;
}

bool MatchingEngineCore::modifyOrder(OrderId orderId, Price newPrice, Quantity newQuantity) {
    std::string symbol;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = orderToSymbol_.find(orderId);
        if (it == orderToSymbol_.end()) {
            return false;
        }
        symbol = it->second;
    }
    
    auto bookIt = orderBooks_.find(symbol);
    if (bookIt == orderBooks_.end()) {
        return false;
    }
    
    return bookIt->second->modifyOrder(orderId, newPrice, newQuantity);
}

OrderPtr MatchingEngineCore::getOrder(OrderId orderId) {
    std::string symbol;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = orderToSymbol_.find(orderId);
        if (it == orderToSymbol_.end()) {
            return nullptr;
        }
        symbol = it->second;
    }
    
    auto bookIt = orderBooks_.find(symbol);
    if (bookIt == orderBooks_.end()) {
        return nullptr;
    }
    
    return bookIt->second->getOrder(orderId);
}

Price MatchingEngineCore::getBestBid(const std::string& symbol) {
    auto it = orderBooks_.find(symbol);
    if (it == orderBooks_.end()) {
        return 0;
    }
    return it->second->getBestBid();
}

Price MatchingEngineCore::getBestAsk(const std::string& symbol) {
    auto it = orderBooks_.find(symbol);
    if (it == orderBooks_.end()) {
        return 0;
    }
    return it->second->getBestAsk();
}

std::vector<std::pair<Price, Quantity>> MatchingEngineCore::getBidDepth(
    const std::string& symbol, size_t levels) {
    auto it = orderBooks_.find(symbol);
    if (it == orderBooks_.end()) {
        return {};
    }
    return it->second->getBidDepth(levels);
}

std::vector<std::pair<Price, Quantity>> MatchingEngineCore::getAskDepth(
    const std::string& symbol, size_t levels) {
    auto it = orderBooks_.find(symbol);
    if (it == orderBooks_.end()) {
        return {};
    }
    return it->second->getAskDepth(levels);
}

void MatchingEngineCore::printOrderBook(const std::string& symbol, size_t levels) {
    auto it = orderBooks_.find(symbol);
    if (it != orderBooks_.end()) {
        it->second->printBook(levels);
    } else {
        std::cout << "Order book for " << symbol << " not found.\n";
    }
}

OrderBook* MatchingEngineCore::getOrCreateOrderBook(const std::string& symbol) {
    auto it = orderBooks_.find(symbol);
    if (it != orderBooks_.end()) {
        return it->second.get();
    }
    
    // Create new order book
    auto book = std::make_unique<OrderBook>(symbol);
    OrderBook* bookPtr = book.get();
    orderBooks_[symbol] = std::move(book);
    
    return bookPtr;
}

void MatchingEngineCore::notifyOrder(const OrderPtr& order) {
    if (orderCallback_) {
        orderCallback_(order);
    }
}

void MatchingEngineCore::notifyTrade(const Trade& trade) {
    if (tradeCallback_) {
        tradeCallback_(trade);
    }
}

} // namespace MatchingEngine

