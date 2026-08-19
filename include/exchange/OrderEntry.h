#pragma once
#include <cstdint>
#include "exchange/Order.h"

namespace exchange
{
    struct OrderEntry
    {
        OrderId id;
        uint64_t remainingQuantity;
    };
}