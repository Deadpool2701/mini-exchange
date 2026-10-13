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

    auto result = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    EXPECT_TRUE(result.trades.empty());

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

    auto result = engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        100,
        100,
        18560
    });

    EXPECT_TRUE(result.trades.empty());

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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    auto result = engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        100,
        100,
        18560
    });

    EXPECT_TRUE(result.trades.empty());

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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    auto result = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    ASSERT_EQ(result.trades.size(), 1);

    EXPECT_EQ(result.trades[0].buyOrderId, exchange::OrderId{101});
    EXPECT_EQ(result.trades[0].sellOrderId, exchange::OrderId{201});
    EXPECT_EQ(result.trades[0].quantity, 100);
    EXPECT_EQ(result.trades[0].priceInCents, 18550);

    EXPECT_TRUE(engine.orderBook().empty());
}

TEST(MatchingEngineTest, BuyOrderMatchesCheaperAsk)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    auto result = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18600
    });

    ASSERT_EQ(result.trades.size(), 1);

    EXPECT_EQ(result.trades[0].quantity, 100);

    // Trade happens at resting ask price
    EXPECT_EQ(result.trades[0].priceInCents, 18550);

    EXPECT_TRUE(engine.orderBook().empty());
}

TEST(MatchingEngineTest, PartialFillLeavesRemainingIncomingOrderInBook)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        50,
        50,
        18550
    });

    auto result = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    ASSERT_EQ(result.trades.size(), 1);

    EXPECT_EQ(result.trades[0].quantity, 50);

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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    auto result = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        50,
        50,
        18550
    });

    ASSERT_EQ(result.trades.size(), 1);

    EXPECT_EQ(result.trades[0].quantity, 50);

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
        exchange::OrderType::Limit,
        50,
        50,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{202},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        50,
        50,
        18550
    });

    auto result = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        75,
        75,
        18550
    });

    ASSERT_EQ(result.trades.size(), 2);

    EXPECT_EQ(
        result.trades[0].sellOrderId,
        exchange::OrderId{201}
    );

    EXPECT_EQ(result.trades[0].quantity, 50);

    EXPECT_EQ(
        result.trades[1].sellOrderId,
        exchange::OrderId{202}
    );

    EXPECT_EQ(result.trades[1].quantity, 25);

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
        exchange::OrderType::Limit,
        50,
        50,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{202},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        50,
        50,
        18560
    });

    engine.submitOrder({
        exchange::OrderId{203},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        50,
        50,
        18570
    });

    auto result = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18570
    });

    ASSERT_EQ(result.trades.size(), 2);

    EXPECT_EQ(result.trades[0].priceInCents, 18550);
    EXPECT_EQ(result.trades[0].quantity, 50);

    EXPECT_EQ(result.trades[1].priceInCents, 18560);
    EXPECT_EQ(result.trades[1].quantity, 50);

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
        exchange::OrderType::Limit,
        50,
        50,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{202},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        50,
        50,
        18600
    });

    auto result = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    ASSERT_EQ(result.trades.size(), 1);

    EXPECT_EQ(result.trades[0].quantity, 50);
    EXPECT_EQ(result.trades[0].priceInCents, 18550);

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
        exchange::OrderType::Limit,
        50,
        50,
        18600
    });

    engine.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        50,
        50,
        18550
    });

    auto result = engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    ASSERT_EQ(result.trades.size(), 2);

    EXPECT_EQ(result.trades[0].priceInCents, 18600);
    EXPECT_EQ(result.trades[0].quantity, 50);

    EXPECT_EQ(result.trades[1].priceInCents, 18550);
    EXPECT_EQ(result.trades[1].quantity, 50);

    EXPECT_TRUE(engine.orderBook().empty());
}

TEST(MatchingEngineTest, CancelsRestingOrder)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        50,
        50,
        18560
    });

    engine.submitOrder({
        exchange::OrderId{202},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        100,
        100,
        18560
    });

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
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

TEST(MatchingEngineTest, SubmitOrderProducesAcceptedEvent)
{
    exchange::MatchingEngine engine("AAPL");

    exchange::Order order{
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    };

    auto result = engine.submitOrder(order);

    ASSERT_EQ(result.events.size(), 1);

    EXPECT_EQ(
        result.events[0].type,
        exchange::OrderEventType::Accepted
    );

    EXPECT_EQ(result.events[0].orderId.value, 101);
    EXPECT_EQ(result.events[0].remainingQuantity, 100);
    EXPECT_EQ(result.events[0].priceInCents, 18550);

    EXPECT_TRUE(result.trades.empty());
}

TEST(MatchingEngineTest, FullyExecutedOrderProducesAcceptedEventAndTrade)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    auto result = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18560
    });

    ASSERT_EQ(result.events.size(), 1);
    EXPECT_EQ(
        result.events[0].type,
        exchange::OrderEventType::Accepted
    );

    ASSERT_EQ(result.trades.size(), 1);

    EXPECT_EQ(result.trades[0].buyOrderId.value, 101);
    EXPECT_EQ(result.trades[0].sellOrderId.value, 201);
    EXPECT_EQ(result.trades[0].quantity, 100);
    EXPECT_EQ(result.trades[0].priceInCents, 18550);
}

TEST(MatchingEngineTest, PartiallyExecutedOrderProducesTradeAndRestsRemainder)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        40,
        40,
        18550
    });

    auto result = engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18560
    });

    ASSERT_EQ(result.events.size(), 1);
    EXPECT_EQ(
        result.events[0].type,
        exchange::OrderEventType::Accepted
    );

    ASSERT_EQ(result.trades.size(), 1);
    EXPECT_EQ(result.trades[0].quantity, 40);

    const exchange::Order* remaining =
        engine.orderBook().findOrder(
            exchange::OrderId{101}
        );

    ASSERT_NE(remaining, nullptr);
    EXPECT_EQ(remaining->remainingQuantity, 60);
}

TEST(MatchingEngineTest, CancelOrderProducesCancelledEvent)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    auto result =
        engine.cancelOrder(exchange::OrderId{101});

    ASSERT_EQ(result.events.size(), 1);
    EXPECT_EQ(
        result.events[0].type,
        exchange::OrderEventType::Cancelled
    );

    EXPECT_EQ(result.events[0].orderId.value, 101);
    EXPECT_EQ(result.events[0].remainingQuantity, 0);
    EXPECT_EQ(result.events[0].priceInCents, 18550);

    EXPECT_TRUE(
        engine.orderBook().findOrder(
            exchange::OrderId{101}
        ) == nullptr
    );
}
TEST(MatchingEngineTest, AmendOrderProducesAmendedEvent)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    auto result = engine.amendOrder(
        exchange::OrderId{101},
        75,
        18560
    );

    ASSERT_EQ(result.events.size(), 1);

    EXPECT_EQ(
        result.events[0].type,
        exchange::OrderEventType::Amended
    );

    EXPECT_EQ(result.events[0].orderId.value, 101);
    EXPECT_EQ(result.events[0].remainingQuantity, 75);
    EXPECT_EQ(result.events[0].priceInCents, 18560);
}

TEST(MatchingEngineTest, BuyMarketOrderPartiallyFillsBestAsk)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        40,
        40,
        18550
    });

    auto result = engine.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Market,
        25,
        25,
        0
    });

    ASSERT_EQ(result.events.size(), 1);
    EXPECT_EQ(
        result.events[0].type,
        exchange::OrderEventType::Accepted
    );

    ASSERT_EQ(result.trades.size(), 1);
    EXPECT_EQ(result.trades[0].buyOrderId, exchange::OrderId{102});
    EXPECT_EQ(result.trades[0].sellOrderId, exchange::OrderId{101});
    EXPECT_EQ(result.trades[0].quantity, 25);
    EXPECT_EQ(result.trades[0].priceInCents, 18550);

    const auto* restingOrder =
        engine.orderBook().findOrder(exchange::OrderId{101});

    ASSERT_NE(restingOrder, nullptr);
    EXPECT_EQ(restingOrder->remainingQuantity, 15);

    const auto* bestAsk = engine.orderBook().bestAsk();

    ASSERT_NE(bestAsk, nullptr);
    EXPECT_EQ(bestAsk->price(), 18550);
    EXPECT_EQ(bestAsk->totalQuantity(), 15);

    EXPECT_EQ(
        engine.orderBook().findOrder(exchange::OrderId{102}),
        nullptr
    );
}

TEST(MatchingEngineTest, SellMarketOrderPartiallyFillsBestBid)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        40,
        40,
        18550
    });

    auto result = engine.submitOrder({
        exchange::OrderId{202},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Market,
        25,
        25,
        0
    });

    ASSERT_EQ(result.trades.size(), 1);
    EXPECT_EQ(result.trades[0].buyOrderId, exchange::OrderId{201});
    EXPECT_EQ(result.trades[0].sellOrderId, exchange::OrderId{202});
    EXPECT_EQ(result.trades[0].quantity, 25);
    EXPECT_EQ(result.trades[0].priceInCents, 18550);

    const auto* restingOrder =
        engine.orderBook().findOrder(exchange::OrderId{201});

    ASSERT_NE(restingOrder, nullptr);
    EXPECT_EQ(restingOrder->remainingQuantity, 15);

    const auto* bestBid = engine.orderBook().bestBid();

    ASSERT_NE(bestBid, nullptr);
    EXPECT_EQ(bestBid->totalQuantity(), 15);

    EXPECT_EQ(
        engine.orderBook().findOrder(exchange::OrderId{202}),
        nullptr
    );
}

TEST(MatchingEngineTest, BuyMarketOrderConsumesMultiplePriceLevels)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{301},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        40,
        40,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{302},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        30,
        30,
        18560
    });

    engine.submitOrder({
        exchange::OrderId{303},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        100,
        100,
        18570
    });

    auto result = engine.submitOrder({
        exchange::OrderId{304},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Market,
        60,
        60,
        0
    });

    ASSERT_EQ(result.trades.size(), 2);

    EXPECT_EQ(result.trades[0].sellOrderId, exchange::OrderId{301});
    EXPECT_EQ(result.trades[0].quantity, 40);
    EXPECT_EQ(result.trades[0].priceInCents, 18550);

    EXPECT_EQ(result.trades[1].sellOrderId, exchange::OrderId{302});
    EXPECT_EQ(result.trades[1].quantity, 20);
    EXPECT_EQ(result.trades[1].priceInCents, 18560);

    EXPECT_EQ(
        engine.orderBook().findOrder(exchange::OrderId{301}),
        nullptr
    );

    const auto* secondOrder =
        engine.orderBook().findOrder(exchange::OrderId{302});

    ASSERT_NE(secondOrder, nullptr);
    EXPECT_EQ(secondOrder->remainingQuantity, 10);

    const auto* thirdOrder =
        engine.orderBook().findOrder(exchange::OrderId{303});

    ASSERT_NE(thirdOrder, nullptr);
    EXPECT_EQ(thirdOrder->remainingQuantity, 100);

    const auto asks = engine.orderBook().asks();

    ASSERT_EQ(asks.size(), 2);
    EXPECT_EQ(asks[0].priceInCents, 18560);
    EXPECT_EQ(asks[0].quantity, 10);
    EXPECT_EQ(asks[1].priceInCents, 18570);
    EXPECT_EQ(asks[1].quantity, 100);
}

TEST(MatchingEngineTest, MarketOrderAgainstEmptyBookDoesNotRest)
{
    exchange::MatchingEngine engine("AAPL");

    auto result = engine.submitOrder({
        exchange::OrderId{401},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Market,
        100,
        100,
        0
    });

    ASSERT_EQ(result.events.size(), 1);
    EXPECT_EQ(
        result.events[0].type,
        exchange::OrderEventType::Accepted
    );

    EXPECT_TRUE(result.trades.empty());
    EXPECT_TRUE(engine.orderBook().empty());

    EXPECT_EQ(
        engine.orderBook().findOrder(exchange::OrderId{401}),
        nullptr
    );

    // A successfully processed order consumes its ID,
    // even when it finds no liquidity.
    EXPECT_THROW(
        engine.submitOrder({
            exchange::OrderId{401},
            "AAPL",
            exchange::Side::Buy,
            exchange::OrderType::Market,
            100,
            100,
            0
        }),
        std::invalid_argument
    );
}

TEST(MatchingEngineTest, MarketOrderDiscardsUnfilledRemainder)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{501},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        40,
        40,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{502},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        30,
        30,
        18560
    });

    auto result = engine.submitOrder({
        exchange::OrderId{503},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Market,
        100,
        100,
        0
    });

    ASSERT_EQ(result.trades.size(), 2);
    EXPECT_EQ(result.trades[0].quantity, 40);
    EXPECT_EQ(result.trades[1].quantity, 30);

    EXPECT_EQ(
        engine.orderBook().findOrder(exchange::OrderId{501}),
        nullptr
    );

    EXPECT_EQ(
        engine.orderBook().findOrder(exchange::OrderId{502}),
        nullptr
    );

    EXPECT_TRUE(engine.orderBook().empty(exchange::Side::Sell));

    EXPECT_EQ(
        engine.orderBook().findOrder(exchange::OrderId{503}),
        nullptr
    );
}

TEST(MatchingEngineTest, MarketOrderPreservesFifoAtSamePrice)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{601},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        40,
        40,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{602},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        30,
        30,
        18550
    });

    auto result = engine.submitOrder({
        exchange::OrderId{603},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Market,
        50,
        50,
        0
    });

    ASSERT_EQ(result.trades.size(), 2);

    EXPECT_EQ(result.trades[0].sellOrderId, exchange::OrderId{601});
    EXPECT_EQ(result.trades[0].quantity, 40);

    EXPECT_EQ(result.trades[1].sellOrderId, exchange::OrderId{602});
    EXPECT_EQ(result.trades[1].quantity, 10);

    EXPECT_EQ(
        engine.orderBook().findOrder(exchange::OrderId{601}),
        nullptr
    );

    const auto* secondOrder =
        engine.orderBook().findOrder(exchange::OrderId{602});

    ASSERT_NE(secondOrder, nullptr);
    EXPECT_EQ(secondOrder->remainingQuantity, 20);

    const auto* bestAsk = engine.orderBook().bestAsk();

    ASSERT_NE(bestAsk, nullptr);
    EXPECT_EQ(bestAsk->totalQuantity(), 20);
}

TEST(MatchingEngineTest, MarketBuyConsumesMultipleAskLevels)
{
    exchange::MatchingEngine engine("AAPL");

    engine.submitOrder({
        exchange::OrderId{901},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        30,
        30,
        18550
    });

    engine.submitOrder({
        exchange::OrderId{902},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        50,
        50,
        18600
    });

    auto result = engine.submitOrder({
        exchange::OrderId{903},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Market,
        60,
        60,
        0
    });

    ASSERT_EQ(result.trades.size(), 2);

    EXPECT_EQ(result.trades[0].quantity, 30);
    EXPECT_EQ(result.trades[0].priceInCents, 18550);

    EXPECT_EQ(result.trades[1].quantity, 30);
    EXPECT_EQ(result.trades[1].priceInCents, 18600);

    const auto& book = engine.orderBook();

    EXPECT_EQ(
        book.findOrder(exchange::OrderId{901}),
        nullptr
    );

    const auto* remainingOrder =
        book.findOrder(exchange::OrderId{902});

    ASSERT_NE(remainingOrder, nullptr);
    EXPECT_EQ(remainingOrder->remainingQuantity, 20);
}

