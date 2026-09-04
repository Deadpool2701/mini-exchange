#include <gtest/gtest.h>

#include "exchange/Trade.h"

TEST(TradeTest, CreatesTrade)
{
    exchange::Trade trade{
        exchange::OrderId{101},
        exchange::OrderId{201},
        50,
        18550
    };

    EXPECT_EQ(trade.buyOrderId, exchange::OrderId{101});
    EXPECT_EQ(trade.sellOrderId, exchange::OrderId{201});
    EXPECT_EQ(trade.quantity, 50);
    EXPECT_EQ(trade.priceInCents, 18550);
}

