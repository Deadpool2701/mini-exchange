#include <gtest/gtest.h>
#include "exchange/PriceLevel.h"
#include "exchange/OrderId.h"
#include "exchange/Order.h"

TEST(PriceLevelTest, CreatesPriceLevel)
{
    exchange::PriceLevel priceLevel(18550);

    EXPECT_EQ(priceLevel.price(), 18550);
    EXPECT_EQ(priceLevel.totalQuantity(), 0);
    EXPECT_TRUE(priceLevel.empty());
}

TEST(PriceLevelTest, AddsOrdersToPriceLevel)
{
    exchange::PriceLevel priceLevel(18550);

    priceLevel.addOrder(exchange::OrderId{1}, 100);
    priceLevel.addOrder(exchange::OrderId{2}, 50);

    EXPECT_EQ(priceLevel.totalQuantity(), 150);
    EXPECT_FALSE(priceLevel.empty());
}

TEST(PriceLevelTest, addOrderThrows)
{
    exchange::PriceLevel priceLevel(18550);

    EXPECT_THROW(
         priceLevel.addOrder(exchange::OrderId{101}, 0),
        std::invalid_argument
    );
}

TEST(PriceLevelTest, OrdersAreMaintainedInFifoOrder)
{
    exchange::PriceLevel priceLevel(18550);

    priceLevel.addOrder(exchange::OrderId{101}, 100);
    priceLevel.addOrder(exchange::OrderId{102}, 50);

    EXPECT_EQ(priceLevel.frontOrder().id.value, 101);
}

TEST(PriceLevelTest, RemovesOrder)
{
    exchange::PriceLevel priceLevel(18550);

    priceLevel.addOrder(exchange::OrderId{101}, 100);
    priceLevel.addOrder(exchange::OrderId{102}, 50);

    priceLevel.removeOrder(exchange::OrderId{101});

    EXPECT_EQ(priceLevel.frontOrder().id.value, 102);
    EXPECT_EQ(priceLevel.totalQuantity(), 50);
}

TEST(PriceLevelTest, RemovingUnknownOrderThrows)
{
    exchange::PriceLevel priceLevel(18550);

    priceLevel.addOrder(exchange::OrderId{101}, 100);

    EXPECT_THROW(
        priceLevel.removeOrder(exchange::OrderId{999}),
        std::invalid_argument
    );
}

TEST(PriceLevelTest, partialFill)
{
    exchange::PriceLevel priceLevel(18550);
    priceLevel.addOrder(exchange::OrderId{101}, 100);

    priceLevel.fill(50);
    EXPECT_EQ(priceLevel.frontOrder().remainingQuantity, 50);
}

TEST(PriceLevelTest, multipleFill)
{
    exchange::PriceLevel priceLevel(18550);
    priceLevel.addOrder(exchange::OrderId{101}, 100);
    priceLevel.addOrder(exchange::OrderId{102}, 50);
    priceLevel.addOrder(exchange::OrderId{103}, 50);

    priceLevel.fill(150);
    EXPECT_EQ(priceLevel.frontOrder().id.value, 103);
}

TEST(PriceLevelTest, multiplePartialFill)
{
    exchange::PriceLevel priceLevel(18550);
    priceLevel.addOrder(exchange::OrderId{101}, 100);
    priceLevel.addOrder(exchange::OrderId{102}, 50);
    priceLevel.addOrder(exchange::OrderId{103}, 50);

    priceLevel.fill(175);
    EXPECT_EQ(priceLevel.frontOrder().remainingQuantity, 25);
}

TEST(PriceLevelTest, failFill)
{
    exchange::PriceLevel priceLevel(18550);
    priceLevel.addOrder(exchange::OrderId{101}, 100);
    priceLevel.addOrder(exchange::OrderId{102}, 50);
    priceLevel.addOrder(exchange::OrderId{103}, 50);

    EXPECT_THROW(
         priceLevel.fill(225),
        std::invalid_argument
    );

    EXPECT_EQ(priceLevel.frontOrder().remainingQuantity, 100);
}
