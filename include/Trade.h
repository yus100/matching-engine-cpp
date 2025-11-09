#pragma once

#include "Common.h"
#include <string>

namespace MatchingEngine {

// Represents an executed trade
class Trade {
public:
    Trade(OrderId buyOrderId,
          OrderId sellOrderId,
          const std::string& symbol,
          Price price,
          Quantity quantity,
          Timestamp timestamp)
        : buyOrderId_(buyOrderId)
        , sellOrderId_(sellOrderId)
        , symbol_(symbol)
        , price_(price)
        , quantity_(quantity)
        , timestamp_(timestamp) {
    }

    OrderId getBuyOrderId() const { return buyOrderId_; }
    OrderId getSellOrderId() const { return sellOrderId_; }
    const std::string& getSymbol() const { return symbol_; }
    Price getPrice() const { return price_; }
    Quantity getQuantity() const { return quantity_; }
    Timestamp getTimestamp() const { return timestamp_; }

    std::string toString() const;

private:
    OrderId buyOrderId_;
    OrderId sellOrderId_;
    std::string symbol_;
    Price price_;
    Quantity quantity_;
    Timestamp timestamp_;
};

} // namespace MatchingEngine

