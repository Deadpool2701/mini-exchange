#include <gtest/gtest.h>

#include "exchange/OrderBook.h"
#include "exchange/OrderId.h"
#include "exchange/Order.h"

TEST(OrderBookTest, CreatesEmptyBook)
{
    exchange::OrderBook book("AAPL");

    EXPECT_TRUE(book.empty());
    EXPECT_TRUE(book.empty(exchange::Side::Buy));
    EXPECT_TRUE(book.empty(exchange::Side::Sell));
}

TEST(OrderBookTest, AddsBuyOrder)
{
    exchange::OrderBook book("AAPL");

    exchange::Order order{
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    };

    book.addOrder(order);

    EXPECT_FALSE(book.empty());
    EXPECT_FALSE(book.empty(exchange::Side::Buy));
    EXPECT_TRUE(book.empty(exchange::Side::Sell));
}

TEST(OrderBookTest, AddsSellOrder)
{
    exchange::OrderBook book("AAPL");

    exchange::Order order{
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18560
    };

    book.addOrder(order);

    EXPECT_FALSE(book.empty());
    EXPECT_TRUE(book.empty(exchange::Side::Buy));
    EXPECT_FALSE(book.empty(exchange::Side::Sell));
}

TEST(OrderBookTest, RejectsOrderForDifferentSymbol)
{
    exchange::OrderBook book("AAPL");

    exchange::Order order{
        exchange::OrderId{101},
        "MSFT",
        exchange::Side::Buy,
        100,
        100,
        18550
    };

    EXPECT_THROW(
        book.addOrder(order),
        std::invalid_argument
    );

    EXPECT_TRUE(book.empty());
}

TEST(OrderBookTest, RejectsZeroQuantityOrder)
{
    exchange::OrderBook book("AAPL");

    exchange::Order order{
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        0,
        0,
        18550
    };

    EXPECT_THROW(
        book.addOrder(order),
        std::invalid_argument
    );

    EXPECT_TRUE(book.empty());
}

TEST(OrderBookTest, BestBidIsNullWhenBookHasNoBids)
{
    exchange::OrderBook book("AAPL");

    EXPECT_EQ(book.bestBid(), nullptr);
}

TEST(OrderBookTest, BestAskIsNullWhenBookHasNoAsks)
{
    exchange::OrderBook book("AAPL");

    EXPECT_EQ(book.bestAsk(), nullptr);
}

TEST(OrderBookTest, BestBidIsHighestPrice)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18540
    });

    book.addOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18560
    });

    book.addOrder({
        exchange::OrderId{103},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    ASSERT_NE(book.bestBid(), nullptr);
    EXPECT_EQ(book.bestBid()->price(), 18560);
}

TEST(OrderBookTest, BestAskIsLowestPrice)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18580
    });

    book.addOrder({
        exchange::OrderId{202},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18560
    });

    book.addOrder({
        exchange::OrderId{203},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18570
    });

    ASSERT_NE(book.bestAsk(), nullptr);
    EXPECT_EQ(book.bestAsk()->price(), 18560);
}

TEST(OrderBookTest, OrdersAtSameBidPriceRemainFifo)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    book.addOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        50,
        50,
        18550
    });

    ASSERT_NE(book.bestBid(), nullptr);

    EXPECT_EQ(
        book.bestBid()->frontOrder(),
        exchange::OrderId{101}
    );
}

TEST(OrderBookTest, CancelsOrder)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    book.cancelOrder(exchange::OrderId{101});

    EXPECT_TRUE(book.empty());
    EXPECT_TRUE(book.empty(exchange::Side::Buy));
}

TEST(OrderBookTest, CancelsOrderWithoutRemovingOtherOrders)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    book.addOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        50,
        50,
        18550
    });

    book.cancelOrder(exchange::OrderId{101});

    ASSERT_NE(book.bestBid(), nullptr);

    EXPECT_EQ(book.bestBid()->price(), 18550);
    EXPECT_EQ(book.bestBid()->totalQuantity(), 50);
    EXPECT_EQ(
        book.bestBid()->frontOrder(),
        exchange::OrderId{102}
    );
}

TEST(OrderBookTest, CancellingLastOrderRemovesPriceLevel)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    book.addOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        50,
        50,
        18560
    });

    book.cancelOrder(exchange::OrderId{102});

    ASSERT_NE(book.bestBid(), nullptr);
    EXPECT_EQ(book.bestBid()->price(), 18550);
}

TEST(OrderBookTest, CancellingUnknownOrderThrows)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    EXPECT_THROW(
        book.cancelOrder(exchange::OrderId{999}),
        std::invalid_argument
    );
}

TEST(OrderBookTest, RejectsDuplicateOrderId)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    EXPECT_THROW(
        book.addOrder({
            exchange::OrderId{101},
            "AAPL",
            exchange::Side::Buy,
            50,
            50,
            18560
        }),
        std::invalid_argument
    );

    ASSERT_NE(book.bestBid(), nullptr);
    EXPECT_EQ(book.bestBid()->price(), 18550);
    EXPECT_EQ(book.bestBid()->totalQuantity(), 100);
}

TEST(OrderBookTest, CancelsSellOrder)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{201},
        "AAPL",
        exchange::Side::Sell,
        100,
        100,
        18560
    });

    book.cancelOrder(exchange::OrderId{201});

    EXPECT_TRUE(book.empty(exchange::Side::Sell));
    EXPECT_EQ(book.bestAsk(), nullptr);
}

TEST(OrderBookTest, CancellingOrderPreservesOtherPriceLevels)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    book.addOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18560
    });

    book.cancelOrder(exchange::OrderId{102});

    ASSERT_NE(book.bestBid(), nullptr);
    EXPECT_EQ(book.bestBid()->price(), 18550);
}

TEST(OrderBookTest, FindsExistingOrder)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    exchange::Order* order =
        book.findOrder(exchange::OrderId{101});

    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->id, exchange::OrderId{101});
    EXPECT_EQ(order->remainingQuantity, 100);
}

TEST(OrderBookTest, ReturnsNullForUnknownOrder)
{
    exchange::OrderBook book("AAPL");

    EXPECT_EQ(
        book.findOrder(exchange::OrderId{999}),
        nullptr
    );
}

TEST(OrderBookTest, FindOrderReturnsMutableOrder)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    exchange::Order* order =
        book.findOrder(exchange::OrderId{101});

    ASSERT_NE(order, nullptr);

    order->fill(30);

    EXPECT_EQ(order->remainingQuantity, 70);
}

TEST(OrderBookTest, PartiallyFillsOrder)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    book.fillOrder(exchange::OrderId{101}, 30);

    const exchange::Order* order =
        book.findOrder(exchange::OrderId{101});

    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->remainingQuantity, 70);

    ASSERT_NE(book.bestBid(), nullptr);

    EXPECT_EQ(
        book.bestBid()->totalQuantity(),
        70
    );
}

TEST(OrderBookTest, FullyFillsAndRemovesOrder)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    book.fillOrder(exchange::OrderId{101}, 100);

    EXPECT_EQ(
        book.findOrder(exchange::OrderId{101}),
        nullptr
    );

    EXPECT_TRUE(book.empty());
}

TEST(OrderBookTest, PartialFillKeepsOrderAtFrontOfPriceLevel)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    book.addOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        50,
        50,
        18550
    });

    book.fillOrder(exchange::OrderId{101}, 30);

    ASSERT_NE(book.bestBid(), nullptr);

    EXPECT_EQ(
        book.bestBid()->frontOrder(),
        exchange::OrderId{101}
    );

    EXPECT_EQ(
        book.bestBid()->totalQuantity(),
        120
    );
}

TEST(OrderBookTest, FullFillMovesToNextOrderInFifoQueue)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    book.addOrder({
        exchange::OrderId{102},
        "AAPL",
        exchange::Side::Buy,
        50,
        50,
        18550
    });

    book.fillOrder(exchange::OrderId{101}, 100);

    ASSERT_NE(book.bestBid(), nullptr);

    EXPECT_EQ(
        book.bestBid()->frontOrder(),
        exchange::OrderId{102}
    );

    EXPECT_EQ(
        book.bestBid()->totalQuantity(),
        50
    );
}

TEST(OrderBookTest, RejectsFillGreaterThanRemainingQuantity)
{
    exchange::OrderBook book("AAPL");

    book.addOrder({
        exchange::OrderId{101},
        "AAPL",
        exchange::Side::Buy,
        100,
        100,
        18550
    });

    EXPECT_THROW(
        book.fillOrder(exchange::OrderId{101}, 101),
        std::invalid_argument
    );

    const exchange::Order* order =
        book.findOrder(exchange::OrderId{101});

    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->remainingQuantity, 100);
}