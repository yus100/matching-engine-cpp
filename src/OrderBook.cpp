#include "OrderBook.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace MatchingEngine {

// PriceLevel implementation
void PriceLevel::addOrder(OrderPtr order) {
    orders_.push_back(order);
    orderMap_[order->getOrderId()] = --orders_.end();
    totalQuantity_ += order->getRemainingQuantity();
}

void PriceLevel::removeOrder(OrderId orderId) {
    auto it = orderMap_.find(orderId);
    if (it != orderMap_.end()) {
        totalQuantity_ -= (*it->second)->getRemainingQuantity();
        orders_.erase(it->second);
        orderMap_.erase(it);
    }
}

OrderPtr PriceLevel::getOrder(OrderId orderId) {
    auto it = orderMap_.find(orderId);
    if (it != orderMap_.end()) {
        return *it->second;
    }
    return nullptr;
}

// OrderBook implementation
OrderBook::OrderBook(const std::string& symbol) 
    : symbol_(symbol) {
}

void OrderBook::addOrder(OrderPtr order) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    orderMap_[order->getOrderId()] = order;
    
    if (order->getSide() == Side::BUY) {
        bids_[order->getPrice()].addOrder(order);
    } else {
        asks_[order->getPrice()].addOrder(order);
    }
}

bool OrderBook::cancelOrder(OrderId orderId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = orderMap_.find(orderId);
    if (it == orderMap_.end()) {
        return false;
    }
    
    OrderPtr order = it->second;
    order->setStatus(OrderStatus::CANCELLED);
    
    if (order->getSide() == Side::BUY) {
        auto levelIt = bids_.find(order->getPrice());
        if (levelIt != bids_.end()) {
            levelIt->second.removeOrder(orderId);
            if (levelIt->second.isEmpty()) {
                bids_.erase(levelIt);
            }
        }
    } else {
        auto levelIt = asks_.find(order->getPrice());
        if (levelIt != asks_.end()) {
            levelIt->second.removeOrder(orderId);
            if (levelIt->second.isEmpty()) {
                asks_.erase(levelIt);
            }
        }
    }
    
    orderMap_.erase(it);
    return true;
}

bool OrderBook::modifyOrder(OrderId orderId, Price newPrice, Quantity newQuantity) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = orderMap_.find(orderId);
    if (it == orderMap_.end()) {
        return false;
    }
    
    OrderPtr order = it->second;
    
    // Remove from current price level
    if (order->getSide() == Side::BUY) {
        auto levelIt = bids_.find(order->getPrice());
        if (levelIt != bids_.end()) {
            levelIt->second.removeOrder(orderId);
            if (levelIt->second.isEmpty()) {
                bids_.erase(levelIt);
            }
        }
    } else {
        auto levelIt = asks_.find(order->getPrice());
        if (levelIt != asks_.end()) {
            levelIt->second.removeOrder(orderId);
            if (levelIt->second.isEmpty()) {
                asks_.erase(levelIt);
            }
        }
    }
    
    // Update order
    order->setPrice(newPrice);
    order->setQuantity(newQuantity);
    order->setStatus(OrderStatus::PENDING);
    
    // Add to new price level (loses time priority)
    if (order->getSide() == Side::BUY) {
        bids_[newPrice].addOrder(order);
    } else {
        asks_[newPrice].addOrder(order);
    }
    
    return true;
}

OrderPtr OrderBook::getOrder(OrderId orderId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = orderMap_.find(orderId);
    if (it != orderMap_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<Trade> OrderBook::matchOrder(OrderPtr order) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    switch (order->getType()) {
        case OrderType::MARKET:
            return matchMarketOrder(order);
        case OrderType::LIMIT:
            return matchLimitOrder(order);
        case OrderType::IOC:
            return matchIOCOrder(order);
        case OrderType::FOK:
            return matchFOKOrder(order);
        case OrderType::STOP_LOSS:
        case OrderType::STOP_LIMIT:
            // Stop orders should be stored and triggered later
            // For now, treat as regular limit orders
            return matchLimitOrder(order);
        default:
            return {};
    }
}

std::vector<Trade> OrderBook::matchMarketOrder(OrderPtr order) {
    std::vector<Trade> trades;
    
    if (order->getSide() == Side::BUY) {
        // Match against asks
        trades = executeMatches(order, asks_, false);
    } else {
        // Match against bids
        trades = executeMatches(order, bids_, false);
    }
    
    // Market orders cancel unfilled portion
    if (order->getRemainingQuantity() > 0) {
        order->setStatus(OrderStatus::CANCELLED);
    }
    
    return trades;
}

std::vector<Trade> OrderBook::matchLimitOrder(OrderPtr order) {
    std::vector<Trade> trades;
    
    if (order->getSide() == Side::BUY) {
        // Buy limit can match with asks at or below limit price
        trades = executeMatches(order, asks_, false);
        
        // If not fully filled, add to book
        if (order->getRemainingQuantity() > 0 && order->isActive()) {
            bids_[order->getPrice()].addOrder(order);
            orderMap_[order->getOrderId()] = order;
        }
    } else {
        // Sell limit can match with bids at or above limit price
        trades = executeMatches(order, bids_, false);
        
        // If not fully filled, add to book
        if (order->getRemainingQuantity() > 0 && order->isActive()) {
            asks_[order->getPrice()].addOrder(order);
            orderMap_[order->getOrderId()] = order;
        }
    }
    
    return trades;
}

std::vector<Trade> OrderBook::matchIOCOrder(OrderPtr order) {
    std::vector<Trade> trades;
    
    if (order->getSide() == Side::BUY) {
        trades = executeMatches(order, asks_, false);
    } else {
        trades = executeMatches(order, bids_, false);
    }
    
    // IOC cancels unfilled portion
    if (order->getRemainingQuantity() > 0) {
        order->setStatus(OrderStatus::CANCELLED);
    }
    
    return trades;
}

std::vector<Trade> OrderBook::matchFOKOrder(OrderPtr order) {
    std::vector<Trade> trades;
    
    // Check if entire order can be filled
    bool canFill = false;
    if (order->getSide() == Side::BUY) {
        canFill = canFillEntireOrder(order, asks_);
    } else {
        canFill = canFillEntireOrder(order, bids_);
    }
    
    if (!canFill) {
        order->setStatus(OrderStatus::CANCELLED);
        return trades;
    }
    
    // Execute the entire order
    if (order->getSide() == Side::BUY) {
        trades = executeMatches(order, asks_, true);
    } else {
        trades = executeMatches(order, bids_, true);
    }
    
    return trades;
}

// Match buy order against sell orders (asks)
std::vector<Trade> OrderBook::executeMatches(
    OrderPtr order, 
    std::map<Price, PriceLevel, std::less<Price>>& levels,
    bool fillEntireOrder) {
    
    std::vector<Trade> trades;
    
    auto it = levels.begin();
    while (it != levels.end() && order->getRemainingQuantity() > 0) {
        // Check price compatibility
        if (order->getType() == OrderType::LIMIT) {
            if (order->getSide() == Side::BUY && it->first > order->getPrice()) {
                break;  // Ask price too high
            }
        }
        
        PriceLevel& level = it->second;
        auto& orders = level.getOrders();
        
        auto orderIt = orders.begin();
        while (orderIt != orders.end() && order->getRemainingQuantity() > 0) {
            OrderPtr matchingOrder = *orderIt;
            
            // Calculate fill quantity
            Quantity fillQty = std::min(order->getRemainingQuantity(), 
                                       matchingOrder->getRemainingQuantity());
            
            // Create trade
            Price tradePrice = matchingOrder->getPrice();  // Passive order price
            Trade trade(
                order->getSide() == Side::BUY ? order->getOrderId() : matchingOrder->getOrderId(),
                order->getSide() == Side::SELL ? order->getOrderId() : matchingOrder->getOrderId(),
                symbol_,
                tradePrice,
                fillQty,
                getCurrentTimestamp()
            );
            trades.push_back(trade);
            
            // Update orders
            order->fill(fillQty);
            matchingOrder->fill(fillQty);
            
            // Remove filled orders
            if (matchingOrder->isFilled()) {
                orderIt = orders.erase(orderIt);
                orderMap_.erase(matchingOrder->getOrderId());
            } else {
                ++orderIt;
            }
        }
        
        // Remove empty price level
        if (level.isEmpty()) {
            it = levels.erase(it);
        } else {
            ++it;
        }
    }
    
    return trades;
}

// Match sell order against buy orders (bids)
std::vector<Trade> OrderBook::executeMatches(
    OrderPtr order, 
    std::map<Price, PriceLevel, std::greater<Price>>& levels,
    bool fillEntireOrder) {
    
    std::vector<Trade> trades;
    
    auto it = levels.begin();
    while (it != levels.end() && order->getRemainingQuantity() > 0) {
        // Check price compatibility
        if (order->getType() == OrderType::LIMIT) {
            if (order->getSide() == Side::SELL && it->first < order->getPrice()) {
                break;  // Bid price too low
            }
        }
        
        PriceLevel& level = it->second;
        auto& orders = level.getOrders();
        
        auto orderIt = orders.begin();
        while (orderIt != orders.end() && order->getRemainingQuantity() > 0) {
            OrderPtr matchingOrder = *orderIt;
            
            // Calculate fill quantity
            Quantity fillQty = std::min(order->getRemainingQuantity(), 
                                       matchingOrder->getRemainingQuantity());
            
            // Create trade
            Price tradePrice = matchingOrder->getPrice();  // Passive order price
            Trade trade(
                order->getSide() == Side::BUY ? order->getOrderId() : matchingOrder->getOrderId(),
                order->getSide() == Side::SELL ? order->getOrderId() : matchingOrder->getOrderId(),
                symbol_,
                tradePrice,
                fillQty,
                getCurrentTimestamp()
            );
            trades.push_back(trade);
            
            // Update orders
            order->fill(fillQty);
            matchingOrder->fill(fillQty);
            
            // Remove filled orders
            if (matchingOrder->isFilled()) {
                orderIt = orders.erase(orderIt);
                orderMap_.erase(matchingOrder->getOrderId());
            } else {
                ++orderIt;
            }
        }
        
        // Remove empty price level
        if (level.isEmpty()) {
            it = levels.erase(it);
        } else {
            ++it;
        }
    }
    
    return trades;
}

bool OrderBook::canFillEntireOrder(
    OrderPtr order, 
    const std::map<Price, PriceLevel, std::less<Price>>& levels) const {
    
    Quantity availableQty = 0;
    
    for (const auto& [price, level] : levels) {
        // Check price compatibility for limit orders
        if (order->getType() == OrderType::LIMIT || order->getType() == OrderType::FOK) {
            if (order->getSide() == Side::BUY && price > order->getPrice()) {
                break;
            }
        }
        
        availableQty += level.getTotalQuantity();
        
        if (availableQty >= order->getRemainingQuantity()) {
            return true;
        }
    }
    
    return false;
}

bool OrderBook::canFillEntireOrder(
    OrderPtr order, 
    const std::map<Price, PriceLevel, std::greater<Price>>& levels) const {
    
    Quantity availableQty = 0;
    
    for (const auto& [price, level] : levels) {
        // Check price compatibility for limit orders
        if (order->getType() == OrderType::LIMIT || order->getType() == OrderType::FOK) {
            if (order->getSide() == Side::SELL && price < order->getPrice()) {
                break;
            }
        }
        
        availableQty += level.getTotalQuantity();
        
        if (availableQty >= order->getRemainingQuantity()) {
            return true;
        }
    }
    
    return false;
}

Price OrderBook::getBestBid() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return bids_.empty() ? 0 : bids_.begin()->first;
}

Price OrderBook::getBestAsk() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return asks_.empty() ? 0 : asks_.begin()->first;
}

Quantity OrderBook::getBidQuantityAtLevel(Price price) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = bids_.find(price);
    return it != bids_.end() ? it->second.getTotalQuantity() : 0;
}

Quantity OrderBook::getAskQuantityAtLevel(Price price) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = asks_.find(price);
    return it != asks_.end() ? it->second.getTotalQuantity() : 0;
}

std::vector<std::pair<Price, Quantity>> OrderBook::getBidDepth(size_t levels) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<Price, Quantity>> depth;
    
    size_t count = 0;
    for (const auto& [price, level] : bids_) {
        if (count >= levels) break;
        depth.emplace_back(price, level.getTotalQuantity());
        ++count;
    }
    
    return depth;
}

std::vector<std::pair<Price, Quantity>> OrderBook::getAskDepth(size_t levels) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<Price, Quantity>> depth;
    
    size_t count = 0;
    for (const auto& [price, level] : asks_) {
        if (count >= levels) break;
        depth.emplace_back(price, level.getTotalQuantity());
        ++count;
    }
    
    return depth;
}

void OrderBook::printBook(size_t levels) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::cout << "\n=== Order Book: " << symbol_ << " ===\n";
    std::cout << std::fixed << std::setprecision(4);
    
    // Print asks (inverted order for visual appeal)
    auto askDepth = getAskDepth(levels);
    for (auto it = askDepth.rbegin(); it != askDepth.rend(); ++it) {
        std::cout << "        "
                  << std::setw(10) << priceToDouble(it->first) << " | "
                  << std::setw(8) << it->second << " ASK\n";
    }
    
    std::cout << "        " << std::string(30, '-') << "\n";
    
    // Print bids
    auto bidDepth = getBidDepth(levels);
    for (const auto& [price, qty] : bidDepth) {
        std::cout << "    BID "
                  << std::setw(8) << qty << " | "
                  << std::setw(10) << priceToDouble(price) << "\n";
    }
    
    std::cout << "==============================\n\n";
}

} // namespace MatchingEngine

