#include <gtest/gtest.h>
#include "Order.h"

using namespace MatchingEngine;

class OrderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for tests
    }
};

// Test Order creation
TEST_F(OrderTest, CreateBasicOrder) {
    Order order(1, "AAPL", Side::BUY, OrderType::LIMIT, 
                doubleToPrice(150.50), 100);
    
    EXPECT_EQ(order.getOrderId(), 1);
    EXPECT_EQ(order.getSymbol(), "AAPL");
    EXPECT_EQ(order.getSide(), Side::BUY);
    EXPECT_EQ(order.getType(), OrderType::LIMIT);
    EXPECT_EQ(order.getPrice(), doubleToPrice(150.50));
    EXPECT_EQ(order.getQuantity(), 100);
    EXPECT_EQ(order.getRemainingQuantity(), 100);
    EXPECT_EQ(order.getFilledQuantity(), 0);
    EXPECT_EQ(order.getStatus(), OrderStatus::PENDING);
}

// Test Order creation with stop price
TEST_F(OrderTest, CreateStopOrder) {
    Order order(1, "AAPL", Side::SELL, OrderType::STOP_LOSS, 
                doubleToPrice(145.00), 50, doubleToPrice(148.00));
    
    EXPECT_EQ(order.getOrderId(), 1);
    EXPECT_EQ(order.getType(), OrderType::STOP_LOSS);
    EXPECT_EQ(order.getStopPrice(), doubleToPrice(148.00));
    EXPECT_EQ(order.getPrice(), doubleToPrice(145.00));
}

// Test order fill operations
TEST_F(OrderTest, FillOrder) {
    Order order(1, "AAPL", Side::BUY, OrderType::LIMIT, 
                doubleToPrice(150.00), 100);
    
    EXPECT_FALSE(order.isFilled());
    EXPECT_TRUE(order.isActive());
    
    order.fill(30);
    EXPECT_EQ(order.getRemainingQuantity(), 70);
    EXPECT_EQ(order.getFilledQuantity(), 30);
    EXPECT_FALSE(order.isFilled());
    EXPECT_EQ(order.getStatus(), OrderStatus::PARTIAL_FILL);
    
    order.fill(70);
    EXPECT_EQ(order.getRemainingQuantity(), 0);
    EXPECT_EQ(order.getFilledQuantity(), 100);
    EXPECT_TRUE(order.isFilled());
    EXPECT_EQ(order.getStatus(), OrderStatus::FILLED);
    EXPECT_FALSE(order.isActive());
}

// Test partial fills
TEST_F(OrderTest, PartialFills) {
    Order order(1, "AAPL", Side::BUY, OrderType::LIMIT, 
                doubleToPrice(150.00), 1000);
    
    order.fill(100);
    EXPECT_EQ(order.getStatus(), OrderStatus::PARTIAL_FILL);
    EXPECT_EQ(order.getFilledQuantity(), 100);
    EXPECT_EQ(order.getRemainingQuantity(), 900);
    
    order.fill(200);
    EXPECT_EQ(order.getFilledQuantity(), 300);
    EXPECT_EQ(order.getRemainingQuantity(), 700);
    
    order.fill(700);
    EXPECT_TRUE(order.isFilled());
    EXPECT_EQ(order.getStatus(), OrderStatus::FILLED);
}

// Test order modification
TEST_F(OrderTest, ModifyOrder) {
    Order order(1, "AAPL", Side::BUY, OrderType::LIMIT, 
                doubleToPrice(150.00), 100);
    
    order.setPrice(doubleToPrice(151.00));
    EXPECT_EQ(order.getPrice(), doubleToPrice(151.00));
    
    order.setQuantity(200);
    EXPECT_EQ(order.getQuantity(), 200);
    EXPECT_EQ(order.getRemainingQuantity(), 200);
}

// Test stop loss trigger (sell side)
TEST_F(OrderTest, StopLossTriggerSell) {
    // Stop loss sell at $148 when price hits or goes below $148
    Order order(1, "AAPL", Side::SELL, OrderType::STOP_LOSS, 
                doubleToPrice(145.00), 100, doubleToPrice(148.00));
    
    // Should not trigger when price is above stop price
    EXPECT_FALSE(order.shouldTrigger(doubleToPrice(149.00)));
    EXPECT_FALSE(order.shouldTrigger(doubleToPrice(148.50)));
    
    // Should trigger when price hits or goes below stop price
    EXPECT_TRUE(order.shouldTrigger(doubleToPrice(148.00)));
    EXPECT_TRUE(order.shouldTrigger(doubleToPrice(147.00)));
}

// Test stop loss trigger (buy side)
TEST_F(OrderTest, StopLossTriggerBuy) {
    // Stop loss buy at $152 when price hits or goes above $152
    Order order(1, "AAPL", Side::BUY, OrderType::STOP_LOSS, 
                doubleToPrice(155.00), 100, doubleToPrice(152.00));
    
    // Should not trigger when price is below stop price
    EXPECT_FALSE(order.shouldTrigger(doubleToPrice(151.00)));
    EXPECT_FALSE(order.shouldTrigger(doubleToPrice(151.50)));
    
    // Should trigger when price hits or goes above stop price
    EXPECT_TRUE(order.shouldTrigger(doubleToPrice(152.00)));
    EXPECT_TRUE(order.shouldTrigger(doubleToPrice(153.00)));
}

// Test stop limit trigger
TEST_F(OrderTest, StopLimitTrigger) {
    Order order(1, "AAPL", Side::SELL, OrderType::STOP_LIMIT, 
                doubleToPrice(148.00), 100, doubleToPrice(150.00));
    
    EXPECT_FALSE(order.shouldTrigger(doubleToPrice(151.00)));
    EXPECT_TRUE(order.shouldTrigger(doubleToPrice(150.00)));
    EXPECT_TRUE(order.shouldTrigger(doubleToPrice(149.00)));
}

// Test order status transitions
TEST_F(OrderTest, StatusTransitions) {
    Order order(1, "AAPL", Side::BUY, OrderType::LIMIT, 
                doubleToPrice(150.00), 100);
    
    EXPECT_EQ(order.getStatus(), OrderStatus::PENDING);
    EXPECT_TRUE(order.isActive());
    
    order.fill(50);
    EXPECT_EQ(order.getStatus(), OrderStatus::PARTIAL_FILL);
    EXPECT_TRUE(order.isActive());
    
    order.fill(50);
    EXPECT_EQ(order.getStatus(), OrderStatus::FILLED);
    EXPECT_FALSE(order.isActive());
    
    Order order2(2, "AAPL", Side::BUY, OrderType::LIMIT, 
                 doubleToPrice(150.00), 100);
    order2.setStatus(OrderStatus::CANCELLED);
    EXPECT_FALSE(order2.isActive());
}

// Test client ID
TEST_F(OrderTest, ClientId) {
    Order order(1, "AAPL", Side::BUY, OrderType::LIMIT, 
                doubleToPrice(150.00), 100);
    
    EXPECT_EQ(order.getClientId(), "");
    
    order.setClientId("client123");
    EXPECT_EQ(order.getClientId(), "client123");
}

// Test different order types
TEST_F(OrderTest, DifferentOrderTypes) {
    Order marketOrder(1, "AAPL", Side::BUY, OrderType::MARKET, 0, 100);
    EXPECT_EQ(marketOrder.getType(), OrderType::MARKET);
    
    Order iocOrder(2, "AAPL", Side::SELL, OrderType::IOC, 
                   doubleToPrice(150.00), 100);
    EXPECT_EQ(iocOrder.getType(), OrderType::IOC);
    
    Order fokOrder(3, "AAPL", Side::BUY, OrderType::FOK, 
                   doubleToPrice(150.00), 100);
    EXPECT_EQ(fokOrder.getType(), OrderType::FOK);
}

// Test price precision
TEST_F(OrderTest, PricePrecision) {
    Price price1 = doubleToPrice(150.1234);
    EXPECT_DOUBLE_EQ(priceToDouble(price1), 150.1234);
    
    Price price2 = doubleToPrice(0.0001);
    EXPECT_DOUBLE_EQ(priceToDouble(price2), 0.0001);
    
    Price price3 = doubleToPrice(9999.9999);
    EXPECT_DOUBLE_EQ(priceToDouble(price3), 9999.9999);
}

