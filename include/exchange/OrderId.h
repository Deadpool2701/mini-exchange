#pragma once

#include <cstdint>
namespace exchange
{
    struct OrderId
    {
        uint64_t value;

        bool operator==(const OrderId& other) const
        {
            return value == other.value;
        }
    };
}