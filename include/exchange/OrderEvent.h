#pragma once

#include <cstdint>

#include <exchange/OrderId.h>

namespace exchange
{
    enum class OrderEventType
    {
        Accepted,
        Amended,
        Cancelled
    };

    struct OrderEvent
    {
        OrderEventType type;
        OrderId orderId;
        uint64_t remainingQuantity;
        int64_t priceInCents;
    };
}