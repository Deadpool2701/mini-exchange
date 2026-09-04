#include <gtest/gtest.h>

#include "exchange/MatchingEngine.h"

TEST(MatchingEngineTest, CreatesEmptyOrderBook)
{
    exchange::MatchingEngine engine("AAPL");

    EXPECT_TRUE(engine.orderBook().empty());
}

TEST(MatchingEngineTest, BuyOrderRestsWhenThereAreNoAsks)
{
    exchange::MatchingEngine engine("AAPL");

    auto trades = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    EXPECT_TRUE(trades.empty());

    ASSERT_NE(engine.orderBook().bestBid(), nullptr);

    EXPECT_EQ(
        engine.orderBook().bestBid()->price(),
        18550
    );

    EXPECT_EQ(
        engine.orderBook().bestBid()->totalQuantity(),
        100
    );
}

TEST(MatchingEngineTest, SellOrderRestsWhenThereAreNoBids)
{
    exchange::MatchingEngine engine("AAPL");

    auto trades = engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18560
    });

    EXPECT_TRUE(trades.empty());

    ASSERT_NE(engine.orderBook().bestAsk(), nullptr);

    EXPECT_EQ(
        engine.orderBook().bestAsk()->price(),
        18560
    );

    EXPECT_EQ(
        engine.orderBook().bestAsk()->totalQuantity(),
        100
    );
}

TEST(MatchingEngineTest, OrdersDoNotMatchWhenPricesDoNotCross)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    auto trades = engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18560
    });

    EXPECT_TRUE(trades.empty());

    ASSERT_NE(engine.orderBook().bestBid(), nullptr);
    ASSERT_NE(engine.orderBook().bestAsk(), nullptr);

    EXPECT_EQ(
        engine.orderBook().bestBid()->totalQuantity(),
        100
    );

    EXPECT_EQ(
        engine.orderBook().bestAsk()->totalQuantity(),
        100
    );
}

TEST(MatchingEngineTest, MatchesOrdersAtSamePrice)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18550
    });

    auto trades = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].buyOrderId, exchange::OrderId{101});
    EXPECT_EQ(trades[0].sellOrderId, exchange::OrderId{201});
    EXPECT_EQ(trades[0].quantity, 100);
    EXPECT_EQ(trades[0].priceInCents, 18550);

    EXPECT_TRUE(engine.orderBook().empty());
}

TEST(MatchingEngineTest, BuyOrderMatchesCheaperAsk)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18550
    });

    auto trades = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18600
    });

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].quantity, 100);

    // Trade happens at resting ask price
    EXPECT_EQ(trades[0].priceInCents, 18550);

    EXPECT_TRUE(engine.orderBook().empty());
}

TEST(MatchingEngineTest, PartialFillLeavesRemainingIncomingOrderInBook)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        50,
        50,
        18550
    });

    auto trades = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].quantity, 50);

    ASSERT_NE(engine.orderBook().bestBid(), nullptr);

    EXPECT_EQ(
        engine.orderBook().bestBid()->price(),
        18550
    );

    EXPECT_EQ(
        engine.orderBook().bestBid()->totalQuantity(),
        50
    );

    EXPECT_EQ(
        engine.orderBook().findOrder(exchange::OrderId{101})
            ->remainingQuantity,
        50
    );
}

TEST(MatchingEngineTest, PartialFillLeavesRemainingRestingOrderInBook)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18550
    });

    auto trades = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        50,
        50,
        18550
    });

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].quantity, 50);

    const exchange::Order* remainingOrder =
        engine.orderBook().findOrder(exchange::OrderId{201});

    ASSERT_NE(remainingOrder, nullptr);

    EXPECT_EQ(
        remainingOrder->remainingQuantity,
        50
    );

    ASSERT_NE(engine.orderBook().bestAsk(), nullptr);

    EXPECT_EQ(
        engine.orderBook().bestAsk()->totalQuantity(),
        50
    );
}

TEST(MatchingEngineTest, MatchesOrdersAtSamePriceInFifoOrder)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        50,
        50,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{202},
        "AAPL",
        exchange::Side::Sell,
        50,
        50,
        18550
    });

    auto trades = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        75,
        75,
        18550
    });

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(
        trades[0].sellOrderId,
        exchange::OrderId{201}
    );

    EXPECT_EQ(trades[0].quantity, 50);

    EXPECT_EQ(
        trades[1].sellOrderId,
        exchange::OrderId{202}
    );

    EXPECT_EQ(trades[1].quantity, 25);

    const exchange::Order* remainingOrder =
        engine.orderBook().findOrder(exchange::OrderId{202});

    ASSERT_NE(remainingOrder, nullptr);

    EXPECT_EQ(
        remainingOrder->remainingQuantity,
        25
    );
}

TEST(MatchingEngineTest, MatchesBestPricesBeforeWorsePrices)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        50,
        50,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{202},
        "AAPL",
        exchange::Side::Sell,
        50,
        50,
        18560
    });

    engine.submitOrder({
        exchange::OrderId{203},
        "AAPL",
        exchange::Side::Sell,
        50,
        50,
        18570
    });

    auto trades = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18570
    });

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0].priceInCents, 18550);
    EXPECT_EQ(trades[0].quantity, 50);

    EXPECT_EQ(trades[1].priceInCents, 18560);
    EXPECT_EQ(trades[1].quantity, 50);

    const exchange::Order* remainingOrder =
        engine.orderBook().findOrder(exchange::OrderId{203});

    ASSERT_NE(remainingOrder, nullptr);

    EXPECT_EQ(
        remainingOrder->remainingQuantity,
        50
    );
}

TEST(MatchingEngineTest, StopsMatchingWhenNextPriceDoesNotCross)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        50,
        50,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{202},
        "AAPL",
        exchange::Side::Sell,
        50,
        50,
        18600
    });

    auto trades = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].quantity, 50);
    EXPECT_EQ(trades[0].priceInCents, 18550);

    ASSERT_NE(engine.orderBook().bestBid(), nullptr);
    ASSERT_NE(engine.orderBook().bestAsk(), nullptr);

    EXPECT_EQ(
        engine.orderBook().bestBid()->totalQuantity(),
        50
    );

    EXPECT_EQ(
        engine.orderBook().bestAsk()->price(),
        18600
    );
}

TEST(MatchingEngineTest, SellOrderMatchesBestBidsFirst)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        50,
        50,
        18600
    });

    engine.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        50,
        50,
        18550
    });

    auto trades = engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18550
    });

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0].priceInCents, 18600);
    EXPECT_EQ(trades[0].quantity, 50);

    EXPECT_EQ(trades[1].priceInCents, 18550);
    EXPECT_EQ(trades[1].quantity, 50);

    EXPECT_TRUE(engine.orderBook().empty());
}

TEST(MatchingEngineTest, CancelsRestingOrder)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    engine.cancelOrder(exchange::OrderId{101});

    EXPECT_TRUE(engine.orderBook().empty());
}

TEST(MatchingEngineTest, CancellingOrderPreservesOtherOrders)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        50,
        50,
        18550
    });

    engine.cancelOrder(exchange::OrderId{101});

    const auto* bestBid = engine.orderBook().bestBid();

    ASSERT_NE(bestBid, nullptr);

    EXPECT_EQ(
        bestBid->frontOrder(),
        exchange::OrderId{102}
    );

    EXPECT_EQ(bestBid->totalQuantity(), 50);
}

TEST(MatchingEngineTest, CancellingUnknownOrderThrows)
{
    exchange::MatchingEngine engine("AAPL");

    EXPECT_THROW(
        engine.cancelOrder(exchange::OrderId{999}),
        std::invalid_argument
    );
}

TEST(MatchingEngineTest, ReducingOrderQuantityPreservesOrder)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    engine.amendOrder(
        exchange::OrderId{101},
        50,
        18550
    );

    const auto* order =
        engine.orderBook().findOrder(exchange::OrderId{101});

    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->remainingQuantity, 50);
    EXPECT_EQ(order->priceInCents, 18550);

    ASSERT_NE(engine.orderBook().bestBid(), nullptr);

    EXPECT_EQ(
        engine.orderBook().bestBid()->totalQuantity(),
        50
    );
}

TEST(MatchingEngineTest, ReducingQuantityPreservesFifoPriority)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    engine.amendOrder(
        exchange::OrderId{101},
        50,
        18550
    );

    ASSERT_NE(engine.orderBook().bestBid(), nullptr);

    EXPECT_EQ(
        engine.orderBook().bestBid()->frontOrder(),
        exchange::OrderId{101}
    );

    EXPECT_EQ(
        engine.orderBook().bestBid()->totalQuantity(),
        150
    );
}

TEST(MatchingEngineTest, IncreasingQuantityPreservesFifoPriority)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    engine.amendOrder(
        exchange::OrderId{101},
        150,
        18550
    );

    ASSERT_NE(engine.orderBook().bestBid(), nullptr);

    EXPECT_EQ(
        engine.orderBook().bestBid()->frontOrder(),
        exchange::OrderId{101}
    );

    EXPECT_EQ(
        engine.orderBook().bestBid()->totalQuantity(),
        250
    );
}

TEST(MatchingEngineTest, ChangingOrderPriceMovesOrderToNewPriceLevel)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    engine.amendOrder(
        exchange::OrderId{101},
        100,
        18560
    );

    const auto* order =
        engine.orderBook().findOrder(exchange::OrderId{101});

    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->priceInCents, 18560);

    ASSERT_NE(engine.orderBook().bestBid(), nullptr);

    EXPECT_EQ(
        engine.orderBook().bestBid()->price(),
        18560
    );
}

TEST(MatchingEngineTest, ChangingPriceAndQuantityUpdatesOrder)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    engine.amendOrder(
        exchange::OrderId{101},
        75,
        18560
    );

    const exchange::Order* order =
        engine.orderBook().findOrder(
            exchange::OrderId{101}
        );

    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->remainingQuantity, 75);
    EXPECT_EQ(order->priceInCents, 18560);

    ASSERT_NE(engine.orderBook().bestBid(), nullptr);

    EXPECT_EQ(
        engine.orderBook().bestBid()->price(),
        18560
    );

    EXPECT_EQ(
        engine.orderBook().bestBid()->totalQuantity(),
        75
    );
}

TEST(MatchingEngineTest, AmendingUnknownOrderThrows)
{
    exchange::MatchingEngine engine("AAPL");

    EXPECT_THROW(
        engine.amendOrder(
            exchange::OrderId{999},
            100,
            18550
        ),
        std::invalid_argument
    );
}

TEST(MatchingEngineTest, AmendingToZeroQuantityThrows)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    EXPECT_THROW(
        engine.amendOrder(
            exchange::OrderId{101},
            0,
            18550
        ),
        std::invalid_argument
    );

    const auto* order =
        engine.orderBook().findOrder(
            exchange::OrderId{101}
        );

    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->remainingQuantity, 100);
}

TEST(MatchingEngineTest, CreatesWithNoTradeHistory)
{
    exchange::MatchingEngine engine("AAPL");

    EXPECT_TRUE(engine.trades().empty());
}

TEST(MatchingEngineTest, StoresCompletedTradeInHistory)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    ASSERT_EQ(engine.trades().size(), 1);

    EXPECT_EQ(
        engine.trades()[0].buyOrderId,
        exchange::OrderId{201}
    );

    EXPECT_EQ(
        engine.trades()[0].sellOrderId,
        exchange::OrderId{101}
    );

    EXPECT_EQ(engine.trades()[0].quantity, 100);
    EXPECT_EQ(engine.trades()[0].priceInCents, 18550);
}

TEST(MatchingEngineTest, TradeHistoryAccumulatesAcrossOrders)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Sell,
        50,
        50,
        18560
    });

    engine.submitOrder({
        exchange::OrderId{202},
        "AAPL",
        exchange::Side::Buy,
        50,
        50,
        18560
    });

    ASSERT_EQ(engine.trades().size(), 2);

    EXPECT_EQ(
        engine.trades()[0].buyOrderId,
        exchange::OrderId{201}
    );

    EXPECT_EQ(
        engine.trades()[1].buyOrderId,
        exchange::OrderId{202}
    );
}

TEST(MatchingEngineTest, StoresMultipleTradesFromSingleIncomingOrder)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18560
    });

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Buy,
        150,
        150,
        18560
    });

    ASSERT_EQ(engine.trades().size(), 2);

    EXPECT_EQ(engine.trades()[0].quantity, 100);
    EXPECT_EQ(engine.trades()[0].priceInCents, 18550);

    EXPECT_EQ(engine.trades()[1].quantity, 50);
    EXPECT_EQ(engine.trades()[1].priceInCents, 18560);
}