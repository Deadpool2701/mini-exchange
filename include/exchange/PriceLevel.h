#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>
#include <deque>
#include <algorithm>
#include <exchange/OrderId.h>


namespace exchange
{
    class PriceLevel
    {
    public:
        explicit PriceLevel(int64_t priceInCents);

        void addOrder(const OrderId& orderId, uint64_t quantity);

        void removeOrder(const OrderId& orderId);

        int64_t price() const;

        uint64_t totalQuantity() const;

        bool empty() const;

        const OrderId& frontOrder() const;

        void reduceQuantity(uint64_t quantity);

        void addQuantity(uint64_t quantity);

    private:
        int64_t priceInCents_;
        std::deque<OrderId> orders_;
        uint64_t totalRemainingQuantity_;
    };
    
} // namespace exchange