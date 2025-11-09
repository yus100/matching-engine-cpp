#include "Order.h"
#include <sstream>
#include <iomanip>

namespace MatchingEngine {

Order::Order(OrderId orderId, 
             const std::string& symbol,
             Side side,
             OrderType type,
             Price price,
             Quantity quantity,
             Price stopPrice)
    : orderId_(orderId)
    , symbol_(symbol)
    , side_(side)
    , type_(type)
    , price_(price)
    , quantity_(quantity)
    , remainingQuantity_(quantity)
    , stopPrice_(stopPrice)
    , status_(OrderStatus::PENDING)
    , timestamp_(getCurrentTimestamp())
    , clientId_("") {
}

void Order::fill(Quantity quantity) {
    if (quantity > remainingQuantity_) {
        quantity = remainingQuantity_;
    }
    
    remainingQuantity_ -= quantity;
    
    if (remainingQuantity_ == 0) {
        status_ = OrderStatus::FILLED;
    } else if (remainingQuantity_ < quantity_) {
        status_ = OrderStatus::PARTIAL_FILL;
    }
}

bool Order::shouldTrigger(Price currentPrice) const {
    if (type_ != OrderType::STOP_LOSS && type_ != OrderType::STOP_LIMIT) {
        return false;
    }
    
    if (side_ == Side::BUY) {
        // Buy stop triggers when price rises to stop price
        return currentPrice >= stopPrice_;
    } else {
        // Sell stop triggers when price falls to stop price
        return currentPrice <= stopPrice_;
    }
}

std::string Order::toString() const {
    std::ostringstream oss;
    oss << "Order[ID=" << orderId_ 
        << ", Symbol=" << symbol_
        << ", Side=" << sideToString(side_)
        << ", Type=" << orderTypeToString(type_)
        << ", Price=" << std::fixed << std::setprecision(4) << priceToDouble(price_)
        << ", Qty=" << quantity_
        << ", Remaining=" << remainingQuantity_
        << ", Status=" << orderStatusToString(status_)
        << "]";
    return oss.str();
}

} // namespace MatchingEngine

