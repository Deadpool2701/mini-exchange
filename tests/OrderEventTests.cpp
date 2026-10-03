#include <gtest/gtest.h>

#include <exchange/OrderEvent.h>

TEST(OrderEventTest, CreatesAcceptedEvent)
{
    exchange::OrderEvent event{
        exchange::OrderEventType::Accepted,
        exchange::OrderId{101},
        100,
        18550
    };

    EXPECT_EQ(
        event.type,
        exchange::OrderEventType::Accepted
    );

    EXPECT_EQ(event.orderId.value, 101);
    EXPECT_EQ(event.remainingQuantity, 100);
    EXPECT_EQ(event.priceInCents, 18550);
}

TEST(OrderEventTest, CreatesAmendedEvent)
{
    exchange::OrderEvent event{
        exchange::OrderEventType::Amended,
        exchange::OrderId{101},
        75,
        18560
    };

    EXPECT_EQ(
        event.type,
        exchange::OrderEventType::Amended
    );

    EXPECT_EQ(event.orderId.value, 101);
    EXPECT_EQ(event.remainingQuantity, 75);
    EXPECT_EQ(event.priceInCents, 18560);
}

TEST(OrderEventTest, CreatesCancelledEvent)
{
    exchange::OrderEvent event{
        exchange::OrderEventType::Cancelled,
        exchange::OrderId{101},
        0,
        18550
    };

    EXPECT_EQ(
        event.type,
        exchange::OrderEventType::Cancelled
    );

    EXPECT_EQ(event.orderId.value, 101);
    EXPECT_EQ(event.remainingQuantity, 0);
    EXPECT_EQ(event.priceInCents, 18550);
}