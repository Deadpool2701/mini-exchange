#pragma once

#include <string>
#include <vector>
#include <unordered_set>

#include "exchange/Order.h"
#include "exchange/OrderBook.h"
#include "exchange/Trade.h"
#include "exchange/OrderResult.h"

namespace exchange
{
    class MatchingEngine
    {
    public:
        explicit MatchingEngine(std::string symbol);

        OrderResult submitOrder(Order order);

        OrderResult cancelOrder(const OrderId& orderId);

        const OrderBook& orderBook() const;

        OrderResult amendOrder(const OrderId& orderId, uint64_t newQuantity, int64_t newPriceInCents );

        const std::vector<Trade>& trades() const;

    private:
        OrderBook orderBook_;
        std::vector<Trade> trades_;
        std::unordered_set<OrderId, OrderIdHash> usedOrderIds_;
    };

} // namespace exchange