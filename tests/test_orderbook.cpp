#include <gtest/gtest.h>
#include "OrderBook.h"
#include <memory>

using namespace MatchingEngine;

class OrderBookTest : public ::testing::Test {
protected:
    void SetUp() override {
        orderBook = std::make_unique<OrderBook>("AAPL");
        nextOrderId = 1;
    }
    
    OrderPtr createOrder(Side side, OrderType type, double price, uint64_t quantity) {
        return std::make_shared<Order>(nextOrderId++, "AAPL", side, type, 
                                       doubleToPrice(price), quantity);
    }
    
    std::unique_ptr<OrderBook> orderBook;
    OrderId nextOrderId;
};

// Test empty order book
TEST_F(OrderBookTest, EmptyOrderBook) {
    EXPECT_EQ(orderBook->getBestBid(), 0);
    EXPECT_EQ(orderBook->getBestAsk(), 0);
    
    auto bidDepth = orderBook->getBidDepth();
    auto askDepth = orderBook->getAskDepth();
    EXPECT_TRUE(bidDepth.empty());
    EXPECT_TRUE(askDepth.empty());
}

// Test adding buy orders
TEST_F(OrderBookTest, AddBuyOrders) {
    auto order1 = createOrder(Side::BUY, OrderType::LIMIT, 150.00, 100);
    auto order2 = createOrder(Side::BUY, OrderType::LIMIT, 149.50, 200);
    auto order3 = createOrder(Side::BUY, OrderType::LIMIT, 150.00, 50);
    
    orderBook->addOrder(order1);
    orderBook->addOrder(order2);
    orderBook->addOrder(order3);
    
    EXPECT_EQ(orderBook->getBestBid(), doubleToPrice(150.00));
    EXPECT_EQ(orderBook->getBidQuantityAtLevel(doubleToPrice(150.00)), 150);
    EXPECT_EQ(orderBook->getBidQuantityAtLevel(doubleToPrice(149.50)), 200);
}

// Test adding sell orders
TEST_F(OrderBookTest, AddSellOrders) {
    auto order1 = createOrder(Side::SELL, OrderType::LIMIT, 151.00, 100);
    auto order2 = createOrder(Side::SELL, OrderType::LIMIT, 151.50, 200);
    auto order3 = createOrder(Side::SELL, OrderType::LIMIT, 151.00, 50);
    
    orderBook->addOrder(order1);
    orderBook->addOrder(order2);
    orderBook->addOrder(order3);
    
    EXPECT_EQ(orderBook->getBestAsk(), doubleToPrice(151.00));
    EXPECT_EQ(orderBook->getAskQuantityAtLevel(doubleToPrice(151.00)), 150);
    EXPECT_EQ(orderBook->getAskQuantityAtLevel(doubleToPrice(151.50)), 200);
}

// Test order book spread
TEST_F(OrderBookTest, OrderBookSpread) {
    auto buyOrder = createOrder(Side::BUY, OrderType::LIMIT, 150.00, 100);
    auto sellOrder = createOrder(Side::SELL, OrderType::LIMIT, 151.00, 100);
    
    orderBook->addOrder(buyOrder);
    orderBook->addOrder(sellOrder);
    
    EXPECT_EQ(orderBook->getBestBid(), doubleToPrice(150.00));
    EXPECT_EQ(orderBook->getBestAsk(), doubleToPrice(151.00));
    
    Price spread = orderBook->getBestAsk() - orderBook->getBestBid();
    EXPECT_EQ(spread, doubleToPrice(1.00));
}

// Test simple limit order matching
TEST_F(OrderBookTest, SimpleLimitOrderMatch) {
    auto sellOrder = createOrder(Side::SELL, OrderType::LIMIT, 150.00, 100);
    orderBook->addOrder(sellOrder);
    
    auto buyOrder = createOrder(Side::BUY, OrderType::LIMIT, 150.00, 100);
    auto trades = orderBook->matchOrder(buyOrder);
    
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].getPrice(), doubleToPrice(150.00));
    EXPECT_EQ(trades[0].getQuantity(), 100);
    EXPECT_EQ(trades[0].getBuyOrderId(), buyOrder->getOrderId());
    EXPECT_EQ(trades[0].getSellOrderId(), sellOrder->getOrderId());
    
    // Order book should be empty after match
    EXPECT_EQ(orderBook->getBestBid(), 0);
    EXPECT_EQ(orderBook->getBestAsk(), 0);
}

// Test partial fill scenario
TEST_F(OrderBookTest, PartialFill) {
    auto sellOrder = createOrder(Side::SELL, OrderType::LIMIT, 150.00, 100);
    orderBook->addOrder(sellOrder);
    
    auto buyOrder = createOrder(Side::BUY, OrderType::LIMIT, 150.00, 50);
    auto trades = orderBook->matchOrder(buyOrder);
    
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].getQuantity(), 50);
    
    // Sell order should still have 50 remaining
    EXPECT_EQ(orderBook->getBestAsk(), doubleToPrice(150.00));
    EXPECT_EQ(orderBook->getAskQuantityAtLevel(doubleToPrice(150.00)), 50);
}

// Test aggressive order fills multiple levels
TEST_F(OrderBookTest, MultiLevelFill) {
    // Add sell orders at different levels
    auto sell1 = createOrder(Side::SELL, OrderType::LIMIT, 150.00, 50);
    auto sell2 = createOrder(Side::SELL, OrderType::LIMIT, 150.50, 50);
    auto sell3 = createOrder(Side::SELL, OrderType::LIMIT, 151.00, 50);
    
    orderBook->addOrder(sell1);
    orderBook->addOrder(sell2);
    orderBook->addOrder(sell3);
    
    // Buy order that crosses multiple levels
    auto buyOrder = createOrder(Side::BUY, OrderType::LIMIT, 151.00, 120);
    auto trades = orderBook->matchOrder(buyOrder);
    
    ASSERT_EQ(trades.size(), 3);
    EXPECT_EQ(trades[0].getPrice(), doubleToPrice(150.00));
    EXPECT_EQ(trades[0].getQuantity(), 50);
    EXPECT_EQ(trades[1].getPrice(), doubleToPrice(150.50));
    EXPECT_EQ(trades[1].getQuantity(), 50);
    EXPECT_EQ(trades[2].getPrice(), doubleToPrice(151.00));
    EXPECT_EQ(trades[2].getQuantity(), 20);
    
    // Remaining quantity at 151.00
    EXPECT_EQ(orderBook->getBestAsk(), doubleToPrice(151.00));
    EXPECT_EQ(orderBook->getAskQuantityAtLevel(doubleToPrice(151.00)), 30);
}

// Test price-time priority
TEST_F(OrderBookTest, PriceTimePriority) {
    // Add three sell orders at same price
    auto sell1 = createOrder(Side::SELL, OrderType::LIMIT, 150.00, 100);
    auto sell2 = createOrder(Side::SELL, OrderType::LIMIT, 150.00, 100);
    auto sell3 = createOrder(Side::SELL, OrderType::LIMIT, 150.00, 100);
    
    orderBook->addOrder(sell1);
    orderBook->addOrder(sell2);
    orderBook->addOrder(sell3);
    
    // Buy order should match in order of submission (FIFO)
    auto buyOrder = createOrder(Side::BUY, OrderType::LIMIT, 150.00, 150);
    auto trades = orderBook->matchOrder(buyOrder);
    
    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].getSellOrderId(), sell1->getOrderId());
    EXPECT_EQ(trades[0].getQuantity(), 100);
    EXPECT_EQ(trades[1].getSellOrderId(), sell2->getOrderId());
    EXPECT_EQ(trades[1].getQuantity(), 50);
    
    // sell3 should still be in the book with full quantity
    EXPECT_EQ(orderBook->getAskQuantityAtLevel(doubleToPrice(150.00)), 150);
}

// Test market order
TEST_F(OrderBookTest, MarketOrder) {
    auto sell1 = createOrder(Side::SELL, OrderType::LIMIT, 150.00, 50);
    auto sell2 = createOrder(Side::SELL, OrderType::LIMIT, 151.00, 50);
    
    orderBook->addOrder(sell1);
    orderBook->addOrder(sell2);
    
    auto marketBuy = createOrder(Side::BUY, OrderType::MARKET, 0, 75);
    auto trades = orderBook->matchOrder(marketBuy);
    
    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].getPrice(), doubleToPrice(150.00));
    EXPECT_EQ(trades[0].getQuantity(), 50);
    EXPECT_EQ(trades[1].getPrice(), doubleToPrice(151.00));
    EXPECT_EQ(trades[1].getQuantity(), 25);
}

// Test IOC (Immediate or Cancel) order
TEST_F(OrderBookTest, IOCOrder) {
    auto sell = createOrder(Side::SELL, OrderType::LIMIT, 150.00, 50);
    orderBook->addOrder(sell);
    
    // IOC order for 100, but only 50 available
    auto iocBuy = createOrder(Side::BUY, OrderType::IOC, 150.00, 100);
    auto trades = orderBook->matchOrder(iocBuy);
    
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].getQuantity(), 50);
    
    // IOC order should not rest in book
    EXPECT_EQ(orderBook->getBestBid(), 0);
}

// Test FOK (Fill or Kill) order - successful fill
TEST_F(OrderBookTest, FOKOrderSuccess) {
    auto sell1 = createOrder(Side::SELL, OrderType::LIMIT, 150.00, 50);
    auto sell2 = createOrder(Side::SELL, OrderType::LIMIT, 150.00, 50);
    
    orderBook->addOrder(sell1);
    orderBook->addOrder(sell2);
    
    // FOK order for 100, and 100 is available
    auto fokBuy = createOrder(Side::BUY, OrderType::FOK, 150.00, 100);
    auto trades = orderBook->matchOrder(fokBuy);
    
    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].getQuantity() + trades[1].getQuantity(), 100);
    
    // All orders should be filled
    EXPECT_EQ(orderBook->getBestAsk(), 0);
}

// Test FOK (Fill or Kill) order - rejected
TEST_F(OrderBookTest, FOKOrderRejected) {
    auto sell = createOrder(Side::SELL, OrderType::LIMIT, 150.00, 50);
    orderBook->addOrder(sell);
    
    // FOK order for 100, but only 50 available
    auto fokBuy = createOrder(Side::BUY, OrderType::FOK, 150.00, 100);
    auto trades = orderBook->matchOrder(fokBuy);
    
    // Should return no trades
    EXPECT_EQ(trades.size(), 0);
    
    // Original sell order should still be in book
    EXPECT_EQ(orderBook->getBestAsk(), doubleToPrice(150.00));
    EXPECT_EQ(orderBook->getAskQuantityAtLevel(doubleToPrice(150.00)), 50);
}

// Test order cancellation
TEST_F(OrderBookTest, CancelOrder) {
    auto order = createOrder(Side::BUY, OrderType::LIMIT, 150.00, 100);
    orderBook->addOrder(order);
    
    EXPECT_EQ(orderBook->getBestBid(), doubleToPrice(150.00));
    
    bool cancelled = orderBook->cancelOrder(order->getOrderId());
    EXPECT_TRUE(cancelled);
    
    EXPECT_EQ(orderBook->getBestBid(), 0);
}

// Test cancel non-existent order
TEST_F(OrderBookTest, CancelNonExistentOrder) {
    bool cancelled = orderBook->cancelOrder(12345);
    EXPECT_FALSE(cancelled);
}

// Test order modification
TEST_F(OrderBookTest, ModifyOrder) {
    auto order = createOrder(Side::BUY, OrderType::LIMIT, 150.00, 100);
    orderBook->addOrder(order);
    
    // Modify price and quantity
    bool modified = orderBook->modifyOrder(order->getOrderId(), 
                                          doubleToPrice(151.00), 200);
    EXPECT_TRUE(modified);
    
    EXPECT_EQ(orderBook->getBestBid(), doubleToPrice(151.00));
    EXPECT_EQ(orderBook->getBidQuantityAtLevel(doubleToPrice(151.00)), 200);
    EXPECT_EQ(orderBook->getBidQuantityAtLevel(doubleToPrice(150.00)), 0);
}

// Test modify non-existent order
TEST_F(OrderBookTest, ModifyNonExistentOrder) {
    bool modified = orderBook->modifyOrder(12345, 
                                          doubleToPrice(150.00), 100);
    EXPECT_FALSE(modified);
}

// Test order retrieval
TEST_F(OrderBookTest, GetOrder) {
    auto order = createOrder(Side::BUY, OrderType::LIMIT, 150.00, 100);
    orderBook->addOrder(order);
    
    auto retrieved = orderBook->getOrder(order->getOrderId());
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getOrderId(), order->getOrderId());
    EXPECT_EQ(retrieved->getPrice(), order->getPrice());
    EXPECT_EQ(retrieved->getQuantity(), order->getQuantity());
}

// Test get non-existent order
TEST_F(OrderBookTest, GetNonExistentOrder) {
    auto retrieved = orderBook->getOrder(12345);
    EXPECT_EQ(retrieved, nullptr);
}

// Test book depth
TEST_F(OrderBookTest, BookDepth) {
    // Add multiple levels
    for (int i = 0; i < 10; i++) {
        auto buy = createOrder(Side::BUY, OrderType::LIMIT, 150.00 - i, 100);
        auto sell = createOrder(Side::SELL, OrderType::LIMIT, 151.00 + i, 100);
        orderBook->addOrder(buy);
        orderBook->addOrder(sell);
    }
    
    auto bidDepth = orderBook->getBidDepth(5);
    auto askDepth = orderBook->getAskDepth(5);
    
    EXPECT_EQ(bidDepth.size(), 5);
    EXPECT_EQ(askDepth.size(), 5);
    
    // Check prices are in correct order
    EXPECT_EQ(bidDepth[0].first, doubleToPrice(150.00));
    EXPECT_EQ(bidDepth[1].first, doubleToPrice(149.00));
    EXPECT_EQ(askDepth[0].first, doubleToPrice(151.00));
    EXPECT_EQ(askDepth[1].first, doubleToPrice(152.00));
}

// Test aggressive sell matching buy orders
TEST_F(OrderBookTest, AggressiveSellMatch) {
    auto buy1 = createOrder(Side::BUY, OrderType::LIMIT, 150.00, 50);
    auto buy2 = createOrder(Side::BUY, OrderType::LIMIT, 149.50, 50);
    
    orderBook->addOrder(buy1);
    orderBook->addOrder(buy2);
    
    auto sellOrder = createOrder(Side::SELL, OrderType::LIMIT, 149.50, 80);
    auto trades = orderBook->matchOrder(sellOrder);
    
    ASSERT_EQ(trades.size(), 2);
    // Should match best bid first
    EXPECT_EQ(trades[0].getPrice(), doubleToPrice(150.00));
    EXPECT_EQ(trades[0].getQuantity(), 50);
    EXPECT_EQ(trades[1].getPrice(), doubleToPrice(149.50));
    EXPECT_EQ(trades[1].getQuantity(), 30);
}

// Test that passive order price is used
TEST_F(OrderBookTest, PassiveOrderPriceUsed) {
    auto sellOrder = createOrder(Side::SELL, OrderType::LIMIT, 150.00, 100);
    orderBook->addOrder(sellOrder);
    
    // Buy at higher price, but should execute at passive order price (150.00)
    auto buyOrder = createOrder(Side::BUY, OrderType::LIMIT, 152.00, 50);
    auto trades = orderBook->matchOrder(buyOrder);
    
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].getPrice(), doubleToPrice(150.00));
}

// Test multiple orders at same price level
TEST_F(OrderBookTest, MultipleOrdersSamePriceLevel) {
    for (int i = 0; i < 5; i++) {
        auto order = createOrder(Side::BUY, OrderType::LIMIT, 150.00, 20);
        orderBook->addOrder(order);
    }
    
    EXPECT_EQ(orderBook->getBestBid(), doubleToPrice(150.00));
    EXPECT_EQ(orderBook->getBidQuantityAtLevel(doubleToPrice(150.00)), 100);
    
    // Sell order matches all
    auto sellOrder = createOrder(Side::SELL, OrderType::LIMIT, 150.00, 100);
    auto trades = orderBook->matchOrder(sellOrder);
    
    EXPECT_EQ(trades.size(), 5);
    EXPECT_EQ(orderBook->getBestBid(), 0);
}

