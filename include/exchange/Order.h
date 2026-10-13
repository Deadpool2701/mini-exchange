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

        void validate() const
        {
            if (id.value == 0)
            {
                throw std::invalid_argument(
                    "Order ID must be non-zero"
                );
            }

            if (symbol.empty())
            {
                throw std::invalid_argument(
                    "Order symbol must not be empty"
                );
            }

            if (side != Side::Buy && side != Side::Sell)
            {
                throw std::invalid_argument(
                    "Invalid order side"
                );
            }

            if (quantity == 0 ||
                remainingQuantity == 0 ||
                remainingQuantity > quantity)
            {
                throw std::invalid_argument(
                    "Invalid order quantity"
                );
            }

            switch (type)
            {
                case OrderType::Limit:
                    if (priceInCents <= 0)
                    {
                        throw std::invalid_argument(
                            "Limit order price must be positive"
                        );
                    }
                    break;

                case OrderType::Market:
                    // Market orders have no limit-price constraint.
                    break;

                default:
                    throw std::invalid_argument(
                        "Invalid order type"
                    );
            }
        }
    };


} // namespace exchange