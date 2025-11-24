// Template for creating new tests
// Copy this file and modify it to add your own test cases

#include <gtest/gtest.h>
#include "Order.h"
#include "OrderBook.h"
#include "MatchingEngine.h"

using namespace MatchingEngine;

// ============================================
// Example 1: Simple Test Without Fixture
// ============================================

TEST(MySimpleTest, BasicTest) {
    // Arrange - Set up test data
    Order order(1, "AAPL", Side::BUY, OrderType::LIMIT, 
                doubleToPrice(150.00), 100);
    
    // Act - Perform the action
    order.fill(50);
    
    // Assert - Verify the results
    EXPECT_EQ(order.getRemainingQuantity(), 50);
    EXPECT_EQ(order.getFilledQuantity(), 50);
}

// ============================================
// Example 2: Test Fixture for Shared Setup
// ============================================

class MyTestFixture : public ::testing::Test {
protected:
    // Called before each test
    void SetUp() override {
        engine = std::make_unique<MatchingEngineCore>();
        
        // Setup callbacks if needed
        engine->setOrderCallback([this](const OrderPtr& order) {
            receivedOrders.push_back(order);
        });
        
        engine->setTradeCallback([this](const Trade& trade) {
            receivedTrades.push_back(trade);
        });
    }
    
    // Called after each test
    void TearDown() override {
        // Cleanup if needed
        engine.reset();
    }
    
    // Helper function to create orders
    OrderPtr createOrder(Side side, double price, uint64_t quantity) {
        return std::make_shared<Order>(
            nextOrderId++, 
            "AAPL", 
            side, 
            OrderType::LIMIT,
            doubleToPrice(price), 
            quantity
        );
    }
    
    // Member variables available to all tests
    std::unique_ptr<MatchingEngineCore> engine;
    std::vector<OrderPtr> receivedOrders;
    std::vector<Trade> receivedTrades;
    OrderId nextOrderId = 1;
};

// Test using the fixture
TEST_F(MyTestFixture, TestWithFixture) {
    // Arrange
    OrderId sellId = engine->submitOrder("AAPL", Side::SELL, OrderType::LIMIT,
                                        doubleToPrice(150.00), 100);
    
    // Act
    OrderId buyId = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT,
                                       doubleToPrice(150.00), 100);
    
    // Assert
    EXPECT_EQ(receivedTrades.size(), 1);
    EXPECT_EQ(receivedTrades[0].getPrice(), doubleToPrice(150.00));
    EXPECT_EQ(receivedTrades[0].getQuantity(), 100);
}

// ============================================
// Example 3: Testing Expected Failures
// ============================================

TEST_F(MyTestFixture, TestExpectedFailure) {
    // Test that canceling non-existent order returns false
    bool result = engine->cancelOrder(999999);
    EXPECT_FALSE(result);
}

// ============================================
// Example 4: Testing Multiple Assertions
// ============================================

TEST_F(MyTestFixture, TestMultipleConditions) {
    engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT,
                       doubleToPrice(150.00), 100);
    
    // Multiple expectations
    EXPECT_EQ(engine->getTotalOrders(), 1);
    EXPECT_EQ(engine->getBestBid("AAPL"), doubleToPrice(150.00));
    EXPECT_EQ(engine->getBestAsk("AAPL"), 0);
    
    auto bidDepth = engine->getBidDepth("AAPL", 10);
    EXPECT_EQ(bidDepth.size(), 1);
    EXPECT_EQ(bidDepth[0].first, doubleToPrice(150.00));
    EXPECT_EQ(bidDepth[0].second, 100);
}

// ============================================
// Example 5: Testing with Assertions
// ============================================

TEST_F(MyTestFixture, TestWithAssertions) {
    OrderId orderId = engine->submitOrder("AAPL", Side::BUY, OrderType::LIMIT,
                                         doubleToPrice(150.00), 100);
    
    auto order = engine->getOrder(orderId);
    
    // ASSERT stops test execution if it fails
    ASSERT_NE(order, nullptr) << "Order should not be null";
    
    // These only run if above assertion passed
    EXPECT_EQ(order->getOrderId(), orderId);
    EXPECT_EQ(order->getPrice(), doubleToPrice(150.00));
}

// ============================================
// Common Assertion Patterns
// ============================================

TEST(AssertionExamples, CommonPatterns) {
    // Equality
    EXPECT_EQ(1, 1);              // Equal
    EXPECT_NE(1, 2);              // Not equal
    
    // Comparison
    EXPECT_LT(1, 2);              // Less than
    EXPECT_LE(1, 1);              // Less than or equal
    EXPECT_GT(2, 1);              // Greater than
    EXPECT_GE(2, 2);              // Greater than or equal
    
    // Boolean
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
    
    // Floating point (with tolerance)
    EXPECT_DOUBLE_EQ(1.0, 1.0);
    EXPECT_NEAR(1.0, 1.001, 0.01);
    
    // Pointer
    int* ptr = nullptr;
    EXPECT_EQ(ptr, nullptr);
    
    // String
    std::string str = "hello";
    EXPECT_EQ(str, "hello");
}

// ============================================
// Example 6: Parameterized Tests
// ============================================

class ParameterizedOrderTest : public ::testing::TestWithParam<OrderType> {
protected:
    void SetUp() override {
        orderType = GetParam();
    }
    
    OrderType orderType;
};

TEST_P(ParameterizedOrderTest, CreateOrderOfEachType) {
    Order order(1, "AAPL", Side::BUY, orderType, 
                doubleToPrice(150.00), 100);
    
    EXPECT_EQ(order.getType(), orderType);
}

// Run this test for each order type
INSTANTIATE_TEST_SUITE_P(
    AllOrderTypes,
    ParameterizedOrderTest,
    ::testing::Values(
        OrderType::MARKET,
        OrderType::LIMIT,
        OrderType::IOC,
        OrderType::FOK,
        OrderType::STOP_LOSS,
        OrderType::STOP_LIMIT
    )
);

// ============================================
// Usage Instructions
// ============================================

/*
To add this file to the test suite:

1. Add it to tests/CMakeLists.txt:
   
   set(TEST_SOURCES
       test_order.cpp
       test_orderbook.cpp
       test_matching_engine.cpp
       test_integration.cpp
       test_template.cpp          # <-- Add this line
   )

2. Rebuild:
   cd build
   cmake ..
   cmake --build .

3. Run your tests:
   ./matching_engine_tests --gtest_filter=My*

Tips:
- Use EXPECT_* for most assertions (test continues)
- Use ASSERT_* for critical checks (test stops)
- Add descriptive test names
- Follow AAA pattern: Arrange, Act, Assert
- Keep tests independent
- Test one thing per test
- Use fixtures for shared setup
*/

