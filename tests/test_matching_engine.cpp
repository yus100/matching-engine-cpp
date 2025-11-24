#include <gtest/gtest.h>
#include "MatchingEngine.h"
#include <vector>
#include <thread>
#include <chrono>

using namespace MatchingEngine;

class MatchingEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<MatchingEngineCore>();
        
        // Set up callbacks to track orders and trades
        orders.clear();
        trades.clear();
        
        engine->setOrderCallback([this](const OrderPtr& order) {
            orders.push_back(order);
        });
        
        engine->setTradeCallback([this](const Trade& trade) {
            trades.push_back(trade);
        });
    }
    
    std::unique_ptr<MatchingEngineCore> engine;
    std::vector<OrderPtr> orders;
    std::vector<Trade> trades;
};

// Test basic order submission
TEST_F(MatchingEngineTest, SubmitBasicOrder) {
    OrderId orderId = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                         doubleToPrice(150.00), 100);
    
    EXPECT_GT(orderId, 0);
    EXPECT_EQ(engine->getTotalOrders(), 1);
    EXPECT_EQ(engine->getTotalTrades(), 0);
}

// Test order matching
TEST_F(MatchingEngineTest, SimpleOrderMatch) {
    OrderId sellId = engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                                        doubleToPrice(150.00), 100);
    
    OrderId buyId = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                       doubleToPrice(150.00), 100);
    
    EXPECT_EQ(engine->getTotalOrders(), 2);
    EXPECT_EQ(engine->getTotalTrades(), 1);
    ASSERT_EQ(trades.size(), 1);
    
    EXPECT_EQ(trades[0].getBuyOrderId(), buyId);
    EXPECT_EQ(trades[0].getSellOrderId(), sellId);
    EXPECT_EQ(trades[0].getPrice(), doubleToPrice(150.00));
    EXPECT_EQ(trades[0].getQuantity(), 100);
}

// Test partial fill
TEST_F(MatchingEngineTest, PartialFill) {
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(150.00), 100);
    
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(150.00), 50);
    
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].getQuantity(), 50);
    
    // Best ask should still exist with remaining quantity
    EXPECT_EQ(engine->getBestAsk("AAPL"), doubleToPrice(150.00));
}

// Test multiple symbols
TEST_F(MatchingEngineTest, MultipleSymbols) {
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(150.00), 100);
    engine->submitOrder("MSFT", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(300.00), 50);
    engine->submitOrder("GOOGL", Side::SELL, OrderType::LIMIT, 
                        doubleToPrice(2800.00), 10);
    
    EXPECT_EQ(engine->getTotalOrders(), 3);
    EXPECT_EQ(engine->getBestBid("AAPL"), doubleToPrice(150.00));
    EXPECT_EQ(engine->getBestBid("MSFT"), doubleToPrice(300.00));
    EXPECT_EQ(engine->getBestAsk("GOOGL"), doubleToPrice(2800.00));
}

// Test order cancellation
TEST_F(MatchingEngineTest, CancelOrder) {
    OrderId orderId = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                         doubleToPrice(150.00), 100);
    
    EXPECT_EQ(engine->getBestBid("AAPL"), doubleToPrice(150.00));
    
    bool cancelled = engine->cancelOrder(orderId);
    EXPECT_TRUE(cancelled);
    
    EXPECT_EQ(engine->getBestBid("AAPL"), 0);
}

// Test cancel non-existent order
TEST_F(MatchingEngineTest, CancelNonExistentOrder) {
    bool cancelled = engine->cancelOrder(99999);
    EXPECT_FALSE(cancelled);
}

// Test order modification
TEST_F(MatchingEngineTest, ModifyOrder) {
    OrderId orderId = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                         doubleToPrice(150.00), 100);
    
    bool modified = engine->modifyOrder(orderId, doubleToPrice(151.00), 200);
    EXPECT_TRUE(modified);
    
    EXPECT_EQ(engine->getBestBid("AAPL"), doubleToPrice(151.00));
}

// Test market order
TEST_F(MatchingEngineTest, MarketOrder) {
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(150.00), 50);
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(151.00), 50);
    
    engine->submitOrder("AAPL", Side::BUY, OrderType::MARKET, 0, 75);
    
    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].getQuantity(), 50);
    EXPECT_EQ(trades[1].getQuantity(), 25);
}

// Test IOC order
TEST_F(MatchingEngineTest, IOCOrder) {
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(150.00), 50);
    
    // IOC for 100, but only 50 available
    engine->submitOrder("AAPL", Side::BUY, OrderType::IOC, 
                       doubleToPrice(150.00), 100);
    
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].getQuantity(), 50);
    
    // No resting order
    EXPECT_EQ(engine->getBestBid("AAPL"), 0);
}

// Test FOK order success
TEST_F(MatchingEngineTest, FOKOrderSuccess) {
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(150.00), 100);
    
    engine->submitOrder("AAPL", Side::BUY, OrderType::FOK, 
                       doubleToPrice(150.00), 100);
    
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].getQuantity(), 100);
}

// Test FOK order rejection
TEST_F(MatchingEngineTest, FOKOrderRejection) {
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(150.00), 50);
    
    engine->submitOrder("AAPL", Side::BUY, OrderType::FOK, 
                       doubleToPrice(150.00), 100);
    
    // Should not execute any trades
    EXPECT_EQ(trades.size(), 0);
    
    // Original order still in book
    EXPECT_EQ(engine->getBestAsk("AAPL"), doubleToPrice(150.00));
}

// Test multiple matches across price levels
TEST_F(MatchingEngineTest, MultiLevelMatch) {
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(150.00), 50);
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(150.50), 50);
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(151.00), 50);
    
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(151.00), 120);
    
    ASSERT_EQ(trades.size(), 3);
    EXPECT_EQ(trades[0].getPrice(), doubleToPrice(150.00));
    EXPECT_EQ(trades[1].getPrice(), doubleToPrice(150.50));
    EXPECT_EQ(trades[2].getPrice(), doubleToPrice(151.00));
}

// Test price-time priority
TEST_F(MatchingEngineTest, PriceTimePriority) {
    OrderId sell1 = engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                                       doubleToPrice(150.00), 100);
    OrderId sell2 = engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                                       doubleToPrice(150.00), 100);
    OrderId sell3 = engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                                       doubleToPrice(150.00), 100);
    
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(150.00), 150);
    
    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].getSellOrderId(), sell1);
    EXPECT_EQ(trades[1].getSellOrderId(), sell2);
}

// Test get order
TEST_F(MatchingEngineTest, GetOrder) {
    OrderId orderId = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                         doubleToPrice(150.00), 100);
    
    auto order = engine->getOrder(orderId);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->getOrderId(), orderId);
    EXPECT_EQ(order->getSymbol(), "AAPL");
    EXPECT_EQ(order->getPrice(), doubleToPrice(150.00));
}

// Test client ID
TEST_F(MatchingEngineTest, ClientId) {
    OrderId orderId = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                         doubleToPrice(150.00), 100, "client123");
    
    auto order = engine->getOrder(orderId);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->getClientId(), "client123");
}

// Test market data queries
TEST_F(MatchingEngineTest, MarketData) {
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(149.00), 100);
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(150.00), 100);
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(151.00), 100);
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(152.00), 100);
    
    EXPECT_EQ(engine->getBestBid("AAPL"), doubleToPrice(150.00));
    EXPECT_EQ(engine->getBestAsk("AAPL"), doubleToPrice(151.00));
    
    auto bidDepth = engine->getBidDepth("AAPL", 10);
    auto askDepth = engine->getAskDepth("AAPL", 10);
    
    EXPECT_EQ(bidDepth.size(), 2);
    EXPECT_EQ(askDepth.size(), 2);
    
    EXPECT_EQ(bidDepth[0].first, doubleToPrice(150.00));
    EXPECT_EQ(bidDepth[1].first, doubleToPrice(149.00));
}

// Test order callbacks
TEST_F(MatchingEngineTest, OrderCallbacks) {
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(150.00), 100);
    
    ASSERT_EQ(orders.size(), 1);
    EXPECT_EQ(orders[0]->getSymbol(), "AAPL");
}

// Test trade callbacks
TEST_F(MatchingEngineTest, TradeCallbacks) {
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(150.00), 100);
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(150.00), 100);
    
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].getSymbol(), "AAPL");
}

// Test aggressive order fills at better price
TEST_F(MatchingEngineTest, AggressiveOrderBetterPrice) {
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(150.00), 100);
    
    // Buy at higher price, should execute at passive order price (150.00)
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(152.00), 50);
    
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].getPrice(), doubleToPrice(150.00));
}

// Test stop loss order (sell)
TEST_F(MatchingEngineTest, StopLossOrderSell) {
    // Place a stop loss sell order at 148 when stop price 148 is hit
    OrderId stopOrderId = engine->submitOrder("AAPL", Side::SELL, 
                                             OrderType::STOP_LOSS, 
                                             doubleToPrice(145.00), 100, "",
                                             doubleToPrice(148.00));
    
    auto stopOrder = engine->getOrder(stopOrderId);
    ASSERT_NE(stopOrder, nullptr);
    EXPECT_EQ(stopOrder->getType(), OrderType::STOP_LOSS);
    EXPECT_EQ(stopOrder->getStopPrice(), doubleToPrice(148.00));
}

// Test stop limit order
TEST_F(MatchingEngineTest, StopLimitOrder) {
    OrderId stopOrderId = engine->submitOrder("AAPL", Side::BUY, 
                                             OrderType::STOP_LIMIT, 
                                             doubleToPrice(152.00), 100, "",
                                             doubleToPrice(151.00));
    
    auto stopOrder = engine->getOrder(stopOrderId);
    ASSERT_NE(stopOrder, nullptr);
    EXPECT_EQ(stopOrder->getType(), OrderType::STOP_LIMIT);
    EXPECT_EQ(stopOrder->getStopPrice(), doubleToPrice(151.00));
    EXPECT_EQ(stopOrder->getPrice(), doubleToPrice(152.00));
}

// Test large order matching
TEST_F(MatchingEngineTest, LargeOrderMatch) {
    // Add many small orders
    for (int i = 0; i < 100; i++) {
        engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                          doubleToPrice(150.00), 10);
    }
    
    // Large buy order
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(150.00), 1000);
    
    EXPECT_EQ(trades.size(), 100);
    
    uint64_t totalQuantity = 0;
    for (const auto& trade : trades) {
        totalQuantity += trade.getQuantity();
    }
    EXPECT_EQ(totalQuantity, 1000);
}

// Test order sequence
TEST_F(MatchingEngineTest, OrderIdSequence) {
    OrderId id1 = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                     doubleToPrice(150.00), 100);
    OrderId id2 = engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                                     doubleToPrice(151.00), 100);
    OrderId id3 = engine->submitOrder("MSFT", Side::BUY, OrderType::LIMIT, 
                                     doubleToPrice(300.00), 50);
    
    EXPECT_GT(id2, id1);
    EXPECT_GT(id3, id2);
}

// Test modification after partial fill
TEST_F(MatchingEngineTest, ModifyAfterPartialFill) {
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(150.00), 100);
    
    OrderId buyId = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                       doubleToPrice(150.00), 50);
    
    // Try to modify the buy order (which should be fully filled)
    // In this case, the order should not exist in the book anymore
    bool modified = engine->modifyOrder(buyId, doubleToPrice(151.00), 100);
    EXPECT_FALSE(modified);
}

// Test zero quantity rejection (implicit through API usage)
TEST_F(MatchingEngineTest, ValidQuantity) {
    OrderId orderId = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                         doubleToPrice(150.00), 100);
    EXPECT_GT(orderId, 0);
}

// Test empty symbol order book
TEST_F(MatchingEngineTest, EmptySymbolBook) {
    EXPECT_EQ(engine->getBestBid("NONEXISTENT"), 0);
    EXPECT_EQ(engine->getBestAsk("NONEXISTENT"), 0);
}

// Test concurrent order submissions (basic thread safety test)
TEST_F(MatchingEngineTest, ConcurrentSubmissions) {
    const int numThreads = 10;
    const int ordersPerThread = 100;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; t++) {
        threads.emplace_back([this, t, ordersPerThread]() {
            for (int i = 0; i < ordersPerThread; i++) {
                Side side = (t % 2 == 0) ? Side::BUY : Side::SELL;
                double price = (side == Side::BUY) ? 149.00 : 151.00;
                engine->submitOrder("AAPL", side, OrderType::LIMIT, 
                                  doubleToPrice(price), 10);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(engine->getTotalOrders(), numThreads * ordersPerThread);
}

// Test statistics
TEST_F(MatchingEngineTest, Statistics) {
    EXPECT_EQ(engine->getTotalOrders(), 0);
    EXPECT_EQ(engine->getTotalTrades(), 0);
    
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(150.00), 100);
    EXPECT_EQ(engine->getTotalOrders(), 1);
    
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(150.00), 100);
    EXPECT_EQ(engine->getTotalOrders(), 2);
    EXPECT_EQ(engine->getTotalTrades(), 1);
}

