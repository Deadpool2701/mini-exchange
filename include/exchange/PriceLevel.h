#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>
#include <deque>
#include <algorithm>
#include <exchange/OrderId.h>
#include <exchange/OrderEntry.h>

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

        void fill(uint64_t quantity);

        bool empty() const;

        const OrderEntry& frontOrder() const;

    private:
        int64_t priceInCents_;
        std::deque<OrderEntry> orders_;
        uint64_t totalQuantity_{0};
    };
    
} // namespace exchange