#include <gtest/gtest.h>

#include "exchange/Exchange.h"
#include "exchange/Order.h"
#include "exchange/OrderId.h"

TEST(ExchangeTest, AddsSymbol)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");

    EXPECT_TRUE(exchange.hasSymbol("AAPL"));
}

TEST(ExchangeTest, AddsMultipleSymbols)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");
    exchange.addSymbol("MSFT");
    exchange.addSymbol("GOOGL");

    EXPECT_TRUE(exchange.hasSymbol("AAPL"));
    EXPECT_TRUE(exchange.hasSymbol("MSFT"));
    EXPECT_TRUE(exchange.hasSymbol("GOOGL"));
}

TEST(ExchangeTest, RejectsDuplicateSymbol)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");

    EXPECT_THROW(
        exchange.addSymbol("AAPL"),
        std::invalid_argument
    );
}

TEST(ExchangeTest, RejectsEmptySymbol)
{
    exchange::Exchange exchange;

    EXPECT_THROW(
        exchange.addSymbol(""),
        std::invalid_argument
    );
}

TEST(ExchangeTest, HasSymbolReturnsFalseForUnknownSymbol)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");

    EXPECT_FALSE(exchange.hasSymbol("MSFT"));
}

TEST(ExchangeTest, ReturnsMatchingEngineForRegisteredSymbol)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");

    const exchange::MatchingEngine& engine =
        exchange.matchingEngine("AAPL");

    EXPECT_TRUE(engine.orderBook().empty());
    EXPECT_EQ(engine.orderBook().symbol(), "AAPL");
}

TEST(ExchangeTest, RetrievingUnknownMatchingEngineThrows)
{
    exchange::Exchange exchange;

    EXPECT_THROW(
        exchange.matchingEngine("AAPL"),
        std::invalid_argument
    );
}

TEST(ExchangeTest, RoutesOrderToCorrectMatchingEngine)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");
    exchange.addSymbol("MSFT");

    exchange.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    EXPECT_FALSE(
        exchange.matchingEngine("AAPL")
            .orderBook()
            .empty()
    );

    EXPECT_TRUE(
        exchange.matchingEngine("MSFT")
            .orderBook()
            .empty()
    );
}

TEST(ExchangeTest, OrdersForDifferentSymbolsDoNotMatch)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");
    exchange.addSymbol("MSFT");

    exchange.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18550
    });

    auto trades = exchange.submitOrder({
        exchange::OrderId{102},
        "MSFT",
        exchange::Side::Buy,
        100,
        100,
        20000
    });

    EXPECT_TRUE(trades.empty());

    EXPECT_FALSE(
        exchange.matchingEngine("AAPL")
            .orderBook()
            .empty()
    );

    EXPECT_FALSE(
        exchange.matchingEngine("MSFT")
            .orderBook()
            .empty()
    );
}

TEST(ExchangeTest, RejectsOrderForUnknownSymbol)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");

    EXPECT_THROW(
        exchange.submitOrder({
            exchange::OrderId{101},
            "MSFT",
            exchange::Side::Buy,
            100,
            100,
            18550
        }),
        std::invalid_argument
    );
}

TEST(ExchangeTest, RejectsDuplicateActiveOrderIdAcrossSymbols)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");
    exchange.addSymbol("MSFT");

    exchange.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    EXPECT_THROW(
        exchange.submitOrder({
            exchange::OrderId{101},
            "MSFT",
            exchange::Side::Buy,
            100,
            100,
            30000
        }),
        std::invalid_argument
    );
}

TEST(ExchangeTest, FullyFilledIncomingOrderCannotBeCancelled)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");

    exchange.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18550
    });

    auto trades = exchange.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    ASSERT_EQ(trades.size(), 1);

    EXPECT_THROW(
        exchange.cancelOrder(exchange::OrderId{102}),
        std::out_of_range
    );
}

TEST(ExchangeTest, FullyFilledRestingOrderCannotBeCancelled)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");

    exchange.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18550
    });

    exchange.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    EXPECT_THROW(
        exchange.cancelOrder(exchange::OrderId{101}),
        std::out_of_range
    );
}

TEST(ExchangeTest, PartiallyFilledRestingOrderCanStillBeCancelled)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");

    exchange.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18550
    });

    exchange.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        40,
        40,
        18550
    });

    EXPECT_NO_THROW(
        exchange.cancelOrder(exchange::OrderId{101})
    );

    EXPECT_TRUE(
        exchange.matchingEngine("AAPL")
            .orderBook()
            .empty()
    );
}

TEST(ExchangeTest, CancelsOrderWithoutKnowingSymbol)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");
    exchange.addSymbol("MSFT");

    exchange.submitOrder({
        exchange::OrderId{101},
        "MSFT",
        exchange::Side::Buy,
        100,
        100,
        30000
    });

    exchange.cancelOrder(exchange::OrderId{101});

    EXPECT_TRUE(
        exchange.matchingEngine("MSFT")
            .orderBook()
            .empty()
    );
}

TEST(ExchangeTest, CancelledOrderCannotBeCancelledAgain)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");

    exchange.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    exchange.cancelOrder(exchange::OrderId{101});

    EXPECT_THROW(
        exchange.cancelOrder(exchange::OrderId{101}),
        std::out_of_range
    );
}

TEST(ExchangeTest, AmendsOrderWithoutKnowingSymbol)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");

    exchange.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    exchange.amendOrder(
        exchange::OrderId{101},
        150,
        18600
    );

    const exchange::Order* order =
        exchange.matchingEngine("AAPL")
            .orderBook()
            .findOrder(exchange::OrderId{101});

    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->remainingQuantity, 150);
    EXPECT_EQ(order->priceInCents, 18600);
}

TEST(ExchangeTest, AmendingUnknownOrderThrows)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");

    EXPECT_THROW(
        exchange.amendOrder(
            exchange::OrderId{999},
            100,
            18550
        ),
        std::out_of_range
    );
}

TEST(ExchangeTest, MatchingOccursIndependentlyPerSymbol)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");
    exchange.addSymbol("MSFT");

    exchange.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18550
    });

    exchange.submitOrder({
        exchange::OrderId{201},
        "MSFT",
        exchange::Side::Sell,
        100,
        100,
        30000
    });

    auto aaplTrades = exchange.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    ASSERT_EQ(aaplTrades.size(), 1);

    EXPECT_FALSE(
        exchange.matchingEngine("MSFT")
            .orderBook()
            .empty()
    );
}

