#pragma once

#include "Common.h"
#include <string>
#include <memory>

namespace MatchingEngine {

class Order {
public:
    Order(OrderId orderId, 
          const std::string& symbol,
          Side side,
          OrderType type,
          Price price,
          Quantity quantity,
          Price stopPrice = 0);

    // Getters
    OrderId getOrderId() const { return orderId_; }
    const std::string& getSymbol() const { return symbol_; }
    Side getSide() const { return side_; }
    OrderType getType() const { return type_; }
    Price getPrice() const { return price_; }
    Quantity getQuantity() const { return quantity_; }
    Quantity getRemainingQuantity() const { return remainingQuantity_; }
    Quantity getFilledQuantity() const { return quantity_ - remainingQuantity_; }
    Price getStopPrice() const { return stopPrice_; }
    OrderStatus getStatus() const { return status_; }
    Timestamp getTimestamp() const { return timestamp_; }
    const std::string& getClientId() const { return clientId_; }

    // Setters
    void setPrice(Price price) { price_ = price; }
    void setQuantity(Quantity quantity) { 
        quantity_ = quantity; 
        remainingQuantity_ = quantity;
    }
    void setStatus(OrderStatus status) { status_ = status; }
    void setClientId(const std::string& clientId) { clientId_ = clientId; }

    // Operations
    void fill(Quantity quantity);
    bool isFilled() const { return remainingQuantity_ == 0; }
    bool isActive() const { 
        return status_ == OrderStatus::PENDING || status_ == OrderStatus::PARTIAL_FILL; 
    }

    // For stop orders - check if should be triggered
    bool shouldTrigger(Price currentPrice) const;

    // Display
    std::string toString() const;

private:
    OrderId orderId_;
    std::string symbol_;
    Side side_;
    OrderType type_;
    Price price_;
    Quantity quantity_;
    Quantity remainingQuantity_;
    Price stopPrice_;  // For stop orders
    OrderStatus status_;
    Timestamp timestamp_;
    std::string clientId_;
};

using OrderPtr = std::shared_ptr<Order>;

} // namespace MatchingEngine

