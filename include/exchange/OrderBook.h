
#pragma once
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <exchange/Order.h>
#include <exchange/PriceLevel.h>
#include <exchange/OrderId.h>

namespace exchange
{
    class OrderBook
    {
        public:

            explicit OrderBook(std::string symbol);

            void addOrder(const Order& order);

            void removeOrder(const OrderId& orderId);

            bool empty() const;

            bool empty(Side side) const;

            const PriceLevel* bestBid() const;
            const PriceLevel* bestAsk() const;

            void cancelOrder(const OrderId& orderId);

            Order* findOrder(const OrderId& orderId);
            
            const Order* findOrder(const OrderId& orderId) const;

            void fillOrder(const OrderId& orderId, uint64_t quantity);

            void amendOrder(const OrderId& orderId, uint64_t newQuantity, int64_t newPriceInCents);

            const std::string& symbol() const;

        private:
            std::string symbol_;
            using BidBook = std::map<int64_t, PriceLevel, std::greater<int64_t>>;
            using AskBook = std::map<int64_t, PriceLevel>;
            BidBook bids_;
            AskBook asks_;

            std::unordered_map<OrderId, Order, OrderIdHash> orders_;
    };
}