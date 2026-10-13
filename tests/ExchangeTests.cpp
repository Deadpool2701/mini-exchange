#include <gtest/gtest.h>

#include "exchange/Exchange.h"
#include "exchange/Order.h"
#include "exchange/OrderId.h"
#include "exchange/ExchangeReplay.h"

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
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    auto result = exchange.submitOrder({
        exchange::OrderId{102},
        "MSFT",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        20000
    });

    EXPECT_TRUE(result.trades.empty());

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
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    EXPECT_THROW(
        exchange.submitOrder({
            exchange::OrderId{101},
            "MSFT",
            exchange::Side::Buy,
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    auto result = exchange.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    ASSERT_EQ(result.trades.size(), 1);

    EXPECT_THROW(
        exchange.cancelOrder(exchange::OrderId{102}),
        std::invalid_argument
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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    exchange.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    EXPECT_THROW(
        exchange.cancelOrder(exchange::OrderId{102}),
        std::invalid_argument
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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    exchange.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    exchange.cancelOrder(exchange::OrderId{101});

    EXPECT_THROW(
        exchange.cancelOrder(exchange::OrderId{102}),
        std::invalid_argument
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
        exchange::OrderType::Limit,
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
        std::invalid_argument
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
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    exchange.submitOrder({
        exchange::OrderId{201},
        "MSFT",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        100,
        100,
        30000
    });

    auto aaplTrades = exchange.submitOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    ASSERT_EQ(aaplTrades.trades.size(), 1);

    EXPECT_FALSE(
        exchange.matchingEngine("MSFT")
            .orderBook()
            .empty()
    );
}

TEST(ExchangeTest, RejectsReuseOfPreviouslyUsedOrderId)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");

    exchange.submitOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    auto result = exchange.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18560
    });

    ASSERT_EQ(result.trades.size(), 1);

    EXPECT_THROW(
        exchange.submitOrder({
            exchange::OrderId{101},
            "AAPL",
            exchange::Side::Buy,
        exchange::OrderType::Limit,
            50,
            50,
            18540
        }),
        std::invalid_argument
    );
}

TEST(ExchangeTest, RejectsReuseOfCancelledOrderId)
{
    exchange::Exchange exchange;

    exchange.addSymbol("AAPL");

    exchange.submitOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    exchange.cancelOrder(
        exchange::OrderId{101}
    );

    EXPECT_THROW(
        exchange.submitOrder({
            exchange::OrderId{101},
            "AAPL",
            exchange::Side::Buy,
        exchange::OrderType::Limit,
            50,
            50,
            18540
        }),
        std::invalid_argument
    );
}

TEST(ExchangeTest, MarketOrderAgainstEmptyBookDoesNotBecomeActive)
{
    exchange::Exchange exchange;
    exchange.addSymbol("AAPL");

    auto result = exchange.submitOrder({
        exchange::OrderId{801},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Market,
        100,
        100,
        0
    });

    EXPECT_TRUE(result.trades.empty());

    const auto& engine = exchange.matchingEngine("AAPL");

    EXPECT_TRUE(engine.orderBook().empty());

    EXPECT_EQ(
        engine.orderBook().findOrder(exchange::OrderId{801}),
        nullptr
    );

    EXPECT_THROW(
        exchange.submitOrder({
            exchange::OrderId{801},
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

TEST(ExchangeTest, MarketOrderRemovesFullyFilledRestingOrder)
{
    exchange::Exchange exchange;
    exchange.addSymbol("AAPL");

    exchange.submitOrder({
        exchange::OrderId{811},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        40,
        40,
        18550
    });

    auto result = exchange.submitOrder({
        exchange::OrderId{812},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Market,
        40,
        40,
        0
    });

    ASSERT_EQ(result.trades.size(), 1);
    EXPECT_EQ(result.trades[0].quantity, 40);

    const auto& engine = exchange.matchingEngine("AAPL");

    EXPECT_EQ(
        engine.orderBook().findOrder(exchange::OrderId{811}),
        nullptr
    );

    EXPECT_TRUE(engine.orderBook().empty());

    // The market order was processed successfully, so its ID
    // cannot be reused even though it never rested in the book.
    EXPECT_THROW(
        exchange.submitOrder({
            exchange::OrderId{812},
            "AAPL",
            exchange::Side::Buy,
            exchange::OrderType::Market,
            40,
            40,
            0
        }),
        std::invalid_argument
    );
}

TEST(ExchangeTest, PartiallyFilledOrderCanBeCancelled)
{
    exchange::Exchange exchange;
    exchange.addSymbol("AAPL");

    // Establish a resting sell order for 100 shares.
    exchange.submitOrder({
        exchange::OrderId{901},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });
    // Buy 40 shares, leaving 60 shares on the ask side.
    auto result = exchange.submitOrder({
        exchange::OrderId{902},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        40,
        40,
        18550
    });
    ASSERT_EQ(result.trades.size(), 1);
    EXPECT_EQ(result.trades[0].quantity, 40);
    const auto& engine = exchange.matchingEngine("AAPL");
    const exchange::Order* remainingOrder =
        engine.orderBook().findOrder(exchange::OrderId{901});
    ASSERT_NE(remainingOrder, nullptr);
    EXPECT_EQ(remainingOrder->remainingQuantity, 60);
    // Cancellation must still work through the Exchange interface.
    auto cancelResult =
        exchange.cancelOrder(exchange::OrderId{901});
    ASSERT_EQ(cancelResult.events.size(), 1);
    EXPECT_EQ(
        cancelResult.events[0].type,
        exchange::OrderEventType::Cancelled
    );
    EXPECT_EQ(
        engine.orderBook().findOrder(exchange::OrderId{901}),
        nullptr
    );
    EXPECT_TRUE(engine.orderBook().empty());

}

TEST(ExchangeTest, FullyFilledOrderCannotBeCancelled)
{
    exchange::Exchange exchange;
    exchange.addSymbol("AAPL");

    // Add a resting sell order.
    exchange.submitOrder({
        exchange::OrderId{911},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        50,
        50,
        18550
    });
    // Completely fill the resting order.
    auto result = exchange.submitOrder({
        exchange::OrderId{912},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        50,
        50,
        18550
    });
    ASSERT_EQ(result.trades.size(), 1);
    EXPECT_EQ(result.trades[0].quantity, 50);
    const auto& engine = exchange.matchingEngine("AAPL");
    EXPECT_EQ(
        engine.orderBook().findOrder(exchange::OrderId{911}),
        nullptr
    );
    EXPECT_TRUE(engine.orderBook().empty());
    // A fully filled order is no longer active.
    EXPECT_THROW(
        exchange.cancelOrder(exchange::OrderId{911}),
        std::invalid_argument
    );

}

TEST(ExchangeTest, FullyFilledOrderCannotBeAmended)
{
    exchange::Exchange exchange;
    exchange.addSymbol("AAPL");

    exchange.submitOrder({
        exchange::OrderId{921},
        "AAPL",
        exchange::Side::Sell,
        exchange::OrderType::Limit,
        50,
        50,
        18550
    });
    auto result = exchange.submitOrder({
        exchange::OrderId{922},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        50,
        50,
        18550
    });
    ASSERT_EQ(result.trades.size(), 1);
    EXPECT_EQ(result.trades[0].quantity, 50);
    const auto& engine = exchange.matchingEngine("AAPL");
    EXPECT_EQ(
        engine.orderBook().findOrder(exchange::OrderId{921}),
        nullptr
    );
    EXPECT_TRUE(engine.orderBook().empty());
    EXPECT_THROW(
        exchange.amendOrder(
            exchange::OrderId{921},
            25,
            18550
        ),
        std::invalid_argument
    );
    EXPECT_TRUE(engine.orderBook().empty());

}

TEST(ExchangeTest, RejectedOrderIdCanBeReused)
{
    exchange::Exchange exchange;
    exchange.addSymbol("AAPL");

    // Invalid limit price: submission must be rejected.
    EXPECT_THROW(
        exchange.submitOrder({
            exchange::OrderId{1301},
            "AAPL",
            exchange::Side::Buy,
            exchange::OrderType::Limit,
            100,
            100,
            0
        }),
        std::invalid_argument
    );

    // The same ID should be available after the rejected submission.
    auto result = exchange.submitOrder({
        exchange::OrderId{1301},
        "AAPL",
        exchange::Side::Buy,
        exchange::OrderType::Limit,
        100,
        100,
        18550
    });

    EXPECT_TRUE(result.trades.empty());

    const auto& engine = exchange.matchingEngine("AAPL");

    const exchange::Order* order =
        engine.orderBook().findOrder(exchange::OrderId{1301});

    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->remainingQuantity, 100);
    EXPECT_EQ(order->priceInCents, 18550);
}

TEST(ExchangeReplayTest, ReconstructsExchangeFromCommands)
{
    const std::vector<exchange::ExchangeCommand> commands{
        exchange::AddSymbolCommand{"AAPL"},
        exchange::AddSymbolCommand{"MSFT"},

        // Resting AAPL sell order.
        exchange::SubmitOrderCommand{
            exchange::Order{
                exchange::OrderId{1701},
                "AAPL",
                exchange::Side::Sell,
                exchange::OrderType::Limit,
                100,
                100,
                18550
            }
        },

        // Executes 40 shares, leaving 60 resting.
        exchange::SubmitOrderCommand{
            exchange::Order{
                exchange::OrderId{1702},
                "AAPL",
                exchange::Side::Buy,
                exchange::OrderType::Limit,
                40,
                40,
                18550
            }
        },

        // Resting MSFT order, subsequently amended and cancelled.
        exchange::SubmitOrderCommand{
            exchange::Order{
                exchange::OrderId{1703},
                "MSFT",
                exchange::Side::Buy,
                exchange::OrderType::Limit,
                10,
                10,
                20000
            }
        },

        exchange::AmendOrderCommand{
            exchange::OrderId{1703},
            10,
            20100
        },

        exchange::CancelOrderCommand{
            exchange::OrderId{1703}
        }
    };

    exchange::Exchange replayed;

    exchange::ExchangeReplay::replay(replayed, commands);

    // Both symbols were reconstructed.
    EXPECT_TRUE(replayed.hasSymbol("AAPL"));
    EXPECT_TRUE(replayed.hasSymbol("MSFT"));

    // The AAPL sell order has 60 shares remaining.
    auto remainingSell =
        replayed.findActiveOrder(exchange::OrderId{1701});

    ASSERT_TRUE(remainingSell.has_value());
    EXPECT_EQ(remainingSell->remainingQuantity, 60);
    EXPECT_EQ(remainingSell->priceInCents, 18550);

    // The incoming buy order was fully filled and is no longer active.
    EXPECT_FALSE(
        replayed.findActiveOrder(exchange::OrderId{1702}).has_value()
    );

    // The MSFT order was amended and then cancelled.
    EXPECT_FALSE(
        replayed.findActiveOrder(exchange::OrderId{1703}).has_value()
    );

    EXPECT_TRUE(replayed.matchingEngine("MSFT").orderBook().empty());

    // Trade history must also be reconstructed.
    const auto& trades = replayed.tradesForSymbol("AAPL");

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].buyOrderId, exchange::OrderId{1702});
    EXPECT_EQ(trades[0].sellOrderId, exchange::OrderId{1701});
    EXPECT_EQ(trades[0].quantity, 40);
    EXPECT_EQ(trades[0].priceInCents, 18550);
}