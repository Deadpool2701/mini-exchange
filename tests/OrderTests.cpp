#include <gtest/gtest.h>

#include "exchange/Order.h"

TEST(OrderTest, CreatesBuyOrder)
{
    exchange::Order order{
        exchange::OrderId{1},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    };

    EXPECT_EQ(order.id.value, 1);
    EXPECT_EQ(order.symbol, "AAPL");
    EXPECT_EQ(order.side, exchange::Side::Buy);
    EXPECT_EQ(order.quantity, 100);
    EXPECT_EQ(order.remainingQuantity, 100);
    EXPECT_EQ(order.priceInCents, 18550);
}

TEST(OrderTest, PartiallyFillsOrder)
{
    exchange::Order order{
        exchange::OrderId{1},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
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
        exchange::OrderId{1},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
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
        exchange::OrderId{1},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    };

    order.fill(100);

    EXPECT_EQ(order.remainingQuantity, 0);
}

TEST(OrderTest, CreatesLimitOrder)
{
    exchange::Order order{
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    };

    EXPECT_EQ(
        order.type,
        exchange::OrderType::Limit
    );
}

TEST(OrderTest, CreatesMarketOrder)
{
    exchange::Order order{
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Market,
        100,
        100,
        0
    };

    EXPECT_EQ(
        order.type,
        exchange::OrderType::Market
    );
}

