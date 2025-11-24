#include <gtest/gtest.h>
#include "MatchingEngine.h"
#include <vector>
#include <random>
#include <algorithm>

using namespace MatchingEngine;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<MatchingEngineCore>();
        
        trades.clear();
        engine->setTradeCallback([this](const Trade& trade) {
            trades.push_back(trade);
        });
    }
    
    std::unique_ptr<MatchingEngineCore> engine;
    std::vector<Trade> trades;
};

// Test complex trading scenario
TEST_F(IntegrationTest, ComplexTradingScenario) {
    // Build a deep order book
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, doubleToPrice(150.00), 100);
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, doubleToPrice(149.50), 200);
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, doubleToPrice(149.00), 150);
    
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, doubleToPrice(151.00), 100);
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, doubleToPrice(151.50), 200);
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, doubleToPrice(152.00), 150);
    
    // Verify spread
    EXPECT_EQ(engine->getBestBid("AAPL"), doubleToPrice(150.00));
    EXPECT_EQ(engine->getBestAsk("AAPL"), doubleToPrice(151.00));
    
    // Large aggressive buy crosses multiple levels
    trades.clear();
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, doubleToPrice(152.00), 350);
    
    ASSERT_EQ(trades.size(), 3);
    
    // Verify trades executed at passive prices
    EXPECT_EQ(trades[0].getPrice(), doubleToPrice(151.00));
    EXPECT_EQ(trades[1].getPrice(), doubleToPrice(151.50));
    EXPECT_EQ(trades[2].getPrice(), doubleToPrice(152.00));
    
    // Verify quantities
    uint64_t totalTraded = 0;
    for (const auto& trade : trades) {
        totalTraded += trade.getQuantity();
    }
    EXPECT_EQ(totalTraded, 350);
}

// Test order book dynamics with cancellations and modifications
TEST_F(IntegrationTest, BookDynamicsWithModifications) {
    OrderId order1 = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                        doubleToPrice(150.00), 100);
    OrderId order2 = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                        doubleToPrice(149.00), 100);
    OrderId order3 = engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                                        doubleToPrice(151.00), 100);
    
    // Cancel middle buy order
    engine->cancelOrder(order1);
    EXPECT_EQ(engine->getBestBid("AAPL"), doubleToPrice(149.00));
    
    // Modify remaining buy order
    engine->modifyOrder(order2, doubleToPrice(150.50), 150);
    EXPECT_EQ(engine->getBestBid("AAPL"), doubleToPrice(150.50));
    
    // Now match
    trades.clear();
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                       doubleToPrice(150.50), 150);
    
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].getQuantity(), 150);
}

// Test mixed order types scenario
TEST_F(IntegrationTest, MixedOrderTypes) {
    // Build book with limit orders
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, doubleToPrice(150.00), 50);
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, doubleToPrice(151.00), 50);
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, doubleToPrice(152.00), 50);
    
    trades.clear();
    
    // Market order takes best available
    engine->submitOrder("AAPL", Side::BUY, OrderType::MARKET, 0, 30);
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].getPrice(), doubleToPrice(150.00));
    
    // IOC order
    trades.clear();
    engine->submitOrder("AAPL", Side::BUY, OrderType::IOC, doubleToPrice(151.00), 60);
    ASSERT_EQ(trades.size(), 2);  // Should fill remaining at 150 and some at 151
    
    // FOK order that succeeds
    trades.clear();
    engine->submitOrder("AAPL", Side::BUY, OrderType::FOK, doubleToPrice(152.00), 50);
    ASSERT_EQ(trades.size(), 2);  // Remaining at 151 and some at 152
}

// Test high-frequency scenario with many small orders
TEST_F(IntegrationTest, HighFrequencyScenario) {
    // Simulate HFT-style rapid order submissions
    for (int i = 0; i < 100; i++) {
        double bidPrice = 150.00 - (i * 0.01);
        double askPrice = 151.00 + (i * 0.01);
        
        engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                          doubleToPrice(bidPrice), 10);
        engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                          doubleToPrice(askPrice), 10);
    }
    
    // Verify book depth
    auto bidDepth = engine->getBidDepth("AAPL", 10);
    auto askDepth = engine->getAskDepth("AAPL", 10);
    
    EXPECT_EQ(bidDepth.size(), 10);
    EXPECT_EQ(askDepth.size(), 10);
    
    // Large sweep order
    trades.clear();
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(155.00), 500);
    
    EXPECT_GT(trades.size(), 0);
    
    uint64_t totalFilled = 0;
    for (const auto& trade : trades) {
        totalFilled += trade.getQuantity();
    }
    EXPECT_EQ(totalFilled, 500);  // Should fill across multiple levels
}

// Test multi-symbol scenario
TEST_F(IntegrationTest, MultiSymbolScenario) {
    // Create order books for multiple symbols
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, doubleToPrice(150.00), 100);
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, doubleToPrice(151.00), 100);
    
    engine->submitOrder("MSFT", Side::BUY, OrderType::LIMIT, doubleToPrice(300.00), 50);
    engine->submitOrder("MSFT", Side::SELL, OrderType::LIMIT, doubleToPrice(301.00), 50);
    
    engine->submitOrder("GOOGL", Side::BUY, OrderType::LIMIT, doubleToPrice(2800.00), 20);
    engine->submitOrder("GOOGL", Side::SELL, OrderType::LIMIT, doubleToPrice(2805.00), 20);
    
    // Verify independent spreads
    EXPECT_EQ(engine->getBestBid("AAPL"), doubleToPrice(150.00));
    EXPECT_EQ(engine->getBestAsk("AAPL"), doubleToPrice(151.00));
    
    EXPECT_EQ(engine->getBestBid("MSFT"), doubleToPrice(300.00));
    EXPECT_EQ(engine->getBestAsk("MSFT"), doubleToPrice(301.00));
    
    EXPECT_EQ(engine->getBestBid("GOOGL"), doubleToPrice(2800.00));
    EXPECT_EQ(engine->getBestAsk("GOOGL"), doubleToPrice(2805.00));
    
    // Execute trades on each symbol
    trades.clear();
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, doubleToPrice(151.00), 50);
    engine->submitOrder("MSFT", Side::SELL, OrderType::LIMIT, doubleToPrice(300.00), 25);
    engine->submitOrder("GOOGL", Side::BUY, OrderType::LIMIT, doubleToPrice(2805.00), 10);
    
    EXPECT_EQ(trades.size(), 3);
    
    // Verify trades are for correct symbols
    int aaplTrades = 0, msftTrades = 0, googlTrades = 0;
    for (const auto& trade : trades) {
        if (trade.getSymbol() == "AAPL") aaplTrades++;
        if (trade.getSymbol() == "MSFT") msftTrades++;
        if (trade.getSymbol() == "GOOGL") googlTrades++;
    }
    
    EXPECT_EQ(aaplTrades, 1);
    EXPECT_EQ(msftTrades, 1);
    EXPECT_EQ(googlTrades, 1);
}

// Test order queue priority with multiple orders at same price
TEST_F(IntegrationTest, OrderQueuePriorityScenario) {
    // Submit multiple orders at same price
    std::vector<OrderId> sellOrders;
    for (int i = 0; i < 10; i++) {
        OrderId id = engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                                        doubleToPrice(150.00), 10);
        sellOrders.push_back(id);
    }
    
    // Buy order that partially fills the queue
    trades.clear();
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(150.00), 55);
    
    // Should match first 6 orders (5 complete + 1 partial)
    ASSERT_EQ(trades.size(), 6);
    
    // Verify FIFO order
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(trades[i].getSellOrderId(), sellOrders[i]);
        EXPECT_EQ(trades[i].getQuantity(), 10);
    }
    
    // Last trade is partial
    EXPECT_EQ(trades[5].getSellOrderId(), sellOrders[5]);
    EXPECT_EQ(trades[5].getQuantity(), 5);
}

// Test iceberg-like scenario (multiple orders at same price, different times)
TEST_F(IntegrationTest, IcebergLikeScenario) {
    // Seller gradually adds liquidity
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, doubleToPrice(150.00), 50);
    
    // Partial buy
    trades.clear();
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, doubleToPrice(150.00), 30);
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].getQuantity(), 30);
    
    // Seller adds more
    engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, doubleToPrice(150.00), 50);
    
    // Another buy fills across both orders
    trades.clear();
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, doubleToPrice(150.00), 60);
    
    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].getQuantity(), 20);  // Remainder of first sell
    EXPECT_EQ(trades[1].getQuantity(), 40);  // Part of second sell
}

// Test self-trade prevention (orders from same client)
TEST_F(IntegrationTest, SameClientOrders) {
    // Note: The basic engine doesn't prevent self-trades
    // This test just verifies behavior
    
    OrderId sellId = engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                                        doubleToPrice(150.00), 100, "client1");
    
    trades.clear();
    OrderId buyId = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                       doubleToPrice(150.00), 50, "client1");
    
    // In a basic engine, this would match
    ASSERT_EQ(trades.size(), 1);
    
    // Retrieve orders to verify client IDs
    auto sellOrder = engine->getOrder(sellId);
    auto buyOrder = engine->getOrder(buyId);
    
    if (sellOrder) {
        EXPECT_EQ(sellOrder->getClientId(), "client1");
    }
    if (buyOrder) {
        EXPECT_EQ(buyOrder->getClientId(), "client1");
    }
}

// Test book rebuild scenario (cancel and resubmit)
TEST_F(IntegrationTest, BookRebuildScenario) {
    // Build initial book
    std::vector<OrderId> orders;
    for (int i = 0; i < 10; i++) {
        OrderId id = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                        doubleToPrice(150.00 - i), 100);
        orders.push_back(id);
    }
    
    // Cancel all orders
    for (auto orderId : orders) {
        engine->cancelOrder(orderId);
    }
    
    EXPECT_EQ(engine->getBestBid("AAPL"), 0);
    
    // Rebuild with different structure
    for (int i = 0; i < 5; i++) {
        engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                          doubleToPrice(151.00 - i), 50);
    }
    
    EXPECT_EQ(engine->getBestBid("AAPL"), doubleToPrice(151.00));
}

// Test aggressive order walks the book
TEST_F(IntegrationTest, AggressiveOrderWalksBook) {
    // Build sell side
    for (int i = 0; i < 20; i++) {
        engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                          doubleToPrice(150.00 + i * 0.10), 100);
    }
    
    // Large aggressive buy
    trades.clear();
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(155.00), 1500);
    
    EXPECT_EQ(trades.size(), 15);  // Should walk through 15 levels
    
    // Verify prices are ascending
    for (size_t i = 1; i < trades.size(); i++) {
        EXPECT_GE(trades[i].getPrice(), trades[i-1].getPrice());
    }
}

// Test market making scenario
TEST_F(IntegrationTest, MarketMakingScenario) {
    // Market maker posts both sides
    OrderId bid1 = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                      doubleToPrice(149.95), 100, "MM1");
    OrderId ask1 = engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                                      doubleToPrice(150.05), 100, "MM1");
    
    // Another market maker with tighter spread
    OrderId bid2 = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                      doubleToPrice(149.98), 50, "MM2");
    OrderId ask2 = engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT, 
                                      doubleToPrice(150.02), 50, "MM2");
    
    // Verify best market is from MM2
    EXPECT_EQ(engine->getBestBid("AAPL"), doubleToPrice(149.98));
    EXPECT_EQ(engine->getBestAsk("AAPL"), doubleToPrice(150.02));
    
    // Retail order hits MM2's ask
    trades.clear();
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                       doubleToPrice(150.02), 50, "retail1");
    
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].getSellOrderId(), ask2);
    
    // Now MM1's ask is best
    EXPECT_EQ(engine->getBestAsk("AAPL"), doubleToPrice(150.05));
}

// Test statistical properties
TEST_F(IntegrationTest, StatisticalProperties) {
    // Generate random orders
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> sideDist(0, 1);
    std::uniform_real_distribution<double> priceDist(148.0, 152.0);
    std::uniform_int_distribution<uint64_t> qtyDist(10, 100);
    
    for (int i = 0; i < 200; i++) {
        Side side = (sideDist(rng) == 0) ? Side::BUY : Side::SELL;
        double price = std::round(priceDist(rng) * 100.0) / 100.0;
        uint64_t qty = qtyDist(rng);
        
        engine->submitOrder("AAPL", side, OrderType::LIMIT, 
                          doubleToPrice(price), qty);
    }
    
    // Verify engine processed all orders
    EXPECT_EQ(engine->getTotalOrders(), 200);
    
    // Should have some trades
    EXPECT_GT(engine->getTotalTrades(), 0);
    
    // Best bid should be less than best ask (or one might be 0)
    Price bestBid = engine->getBestBid("AAPL");
    Price bestAsk = engine->getBestAsk("AAPL");
    
    if (bestBid > 0 && bestAsk > 0) {
        EXPECT_LT(bestBid, bestAsk);
    }
}

// Test stress scenario - many modifications
TEST_F(IntegrationTest, StressModifications) {
    std::vector<OrderId> orders;
    
    // Submit 50 orders
    for (int i = 0; i < 50; i++) {
        OrderId id = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT, 
                                        doubleToPrice(150.00 - i * 0.10), 100);
        orders.push_back(id);
    }
    
    // Modify all orders
    for (auto orderId : orders) {
        auto order = engine->getOrder(orderId);
        if (order) {
            double newPrice = priceToDouble(order->getPrice()) + 0.50;
            engine->modifyOrder(orderId, doubleToPrice(newPrice), 
                              order->getQuantity() + 50);
        }
    }
    
    // Verify best bid increased
    EXPECT_GE(engine->getBestBid("AAPL"), doubleToPrice(150.00));
}

