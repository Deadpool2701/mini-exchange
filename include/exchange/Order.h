#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>
#include <exchange/OrderId.h>

namespace exchange
{
    enum class OrderType
    {
        Limit,
        Market
    };

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
        OrderType type;
        uint64_t quantity;
        uint64_t remainingQuantity;
        int64_t priceInCents;

        void fill(uint64_t fillQuantity)
        {
            if (fillQuantity == 0)
            {
                throw std::invalid_argument("Fill quantity must be greater than zero");
            }
            if(remainingQuantity < fillQuantity)
                throw std::invalid_argument("Fill quantity exceeds remaining order quantity");

            remainingQuantity -= fillQuantity;
        }
    };


} // namespace exchange