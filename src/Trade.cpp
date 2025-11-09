#include "Trade.h"
#include <sstream>
#include <iomanip>

namespace MatchingEngine {

std::string Trade::toString() const {
    std::ostringstream oss;
    oss << "Trade[Buy=" << buyOrderId_ 
        << ", Sell=" << sellOrderId_
        << ", Symbol=" << symbol_
        << ", Price=" << std::fixed << std::setprecision(4) << priceToDouble(price_)
        << ", Qty=" << quantity_
        << "]";
    return oss.str();
}

} // namespace MatchingEngine

