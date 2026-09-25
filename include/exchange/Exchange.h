#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "exchange/MatchingEngine.h"
#include "exchange/OrderId.h"
#include "exchange/Trade.h"

namespace exchange
{
    class Exchange
    {
    public:
        void addSymbol(std::string symbol);

        bool hasSymbol(const std::string& symbol) const;

        const MatchingEngine& matchingEngine(const std::string& symbol) const;

        std::vector<Trade> submitOrder(Order order);

        void cancelOrder(const OrderId& orderId);

        void amendOrder(const OrderId& orderId, uint64_t newQuantity, int64_t newPriceInCents);

    private:
        std::unordered_map<std::string, MatchingEngine> engines_;

        std::unordered_map<OrderId, std::string, OrderIdHash> orderSymbols_;
    };
}