#include <gtest/gtest.h>

#include "exchange/Order.h"

TEST(OrderTest, CreatesBuyOrder)
{
    exchange::Order order{
        1,
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    };

    EXPECT_EQ(order.id, 1);
    EXPECT_EQ(order.symbol, "AAPL");
    EXPECT_EQ(order.side, exchange::Side::Buy);
    EXPECT_EQ(order.quantity, 100);
    EXPECT_EQ(order.remainingQuantity, 100);
    EXPECT_EQ(order.priceInCents, 18550);
}

TEST(OrderTest, PartiallyFillsOrder)
{
    exchange::Order order{
        1,
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    };

    order.fill(30);

    EXPECT_EQ(order.quantity, 100);
    EXPECT_EQ(order.remainingQuantity, 70);
}

TEST(OrderTest, RejectsFillExceedingRemainingQuantity)
{
    exchange::Order order{
        1,
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    };

    EXPECT_THROW(order.fill(101), std::invalid_argument);

    EXPECT_EQ(order.remainingQuantity, 100);
}

TEST(OrderTest, FullyFillsOrder)
{
    exchange::Order order{
        1,
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    };

    order.fill(100);

    EXPECT_EQ(order.remainingQuantity, 0);
}