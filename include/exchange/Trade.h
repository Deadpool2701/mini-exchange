#pragma once

#include <cstdint>

#include "exchange/OrderId.h"

namespace exchange
{
    struct Trade
    {
        OrderId buyOrderId;
        OrderId sellOrderId;
        uint64_t quantity;
        int64_t priceInCents;
    };
}