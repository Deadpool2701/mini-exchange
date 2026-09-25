#pragma once

#include <cstdint>

namespace exchange
{
    struct BookLevel
    {
        int64_t priceInCents;
        uint64_t quantity;
    };
}