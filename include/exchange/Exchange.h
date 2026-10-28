#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <optional>
#include <mutex>

#include "exchange/MatchingEngine.h"
#include "exchange/OrderId.h"
#include "exchange/Trade.h"
#include "exchange/OrderResult.h"

namespace exchange
{
    class Exchange
    {
    public:
        void addSymbol(std::string symbol);

        bool hasSymbol(const std::string& symbol) const;

        const MatchingEngine& matchingEngine(const std::string& symbol) const;

        OrderResult submitOrder(Order order);

        OrderResult cancelOrder(const OrderId& orderId);

        OrderResult amendOrder(const OrderId& orderId, uint64_t newQuantity, int64_t newPriceInCents);

        std::optional<Order> findActiveOrder( const OrderId& orderId) const;

        const std::vector<Trade>& tradesForSymbol( const std::string& symbol ) const;

    private:
        mutable std::mutex mutex_;
        std::unordered_map<std::string, MatchingEngine> engines_;
        std::unordered_map<OrderId, std::string, OrderIdHash> orderSymbols_;
        std::unordered_set<OrderId, OrderIdHash> usedOrderIds_;
    };
}