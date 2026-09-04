#pragma once

#include <string>
#include <vector>

#include "exchange/Order.h"
#include "exchange/OrderBook.h"
#include "exchange/Trade.h"

namespace exchange
{
    class MatchingEngine
    {
    public:
        explicit MatchingEngine(std::string symbol);

        std::vector<Trade> submitOrder(Order order);

        void cancelOrder(const OrderId& orderId);

        const OrderBook& orderBook() const;

        void amendOrder(const OrderId& orderId, uint64_t newQuantity, int64_t newPriceInCents );

        const std::vector<Trade>& trades() const;

    private:
        OrderBook orderBook_;
        std::vector<Trade> trades_;
    };

} // namespace exchange