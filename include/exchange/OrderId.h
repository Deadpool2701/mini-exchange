#pragma once

#include <cstdint>
#include <functional>

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

    struct OrderIdHash
    {
        std::size_t operator()(const OrderId& id) const noexcept
        {
            return std::hash<uint64_t>{}(id.value);
        }
    };
}