#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>
#include <exchange/OrderId.h>

namespace exchange
{
    enum class Side
    {
        Buy,
        Sell
    };

    struct Order
    {
        OrderId id;
        std::string symbol;
        Side side;
        uint64_t quantity;
        uint64_t remainingQuantity;
        int64_t priceInCents;

        void fill(uint64_t fillQuantity)
        {
            if(remainingQuantity < fillQuantity)
                throw std::invalid_argument("Fill quantity exceeds remaining order quantity");

            remainingQuantity -= fillQuantity;
        }
    };


} // namespace exchange