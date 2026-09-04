#include "exchange/OrderBook.h"

#include <stdexcept>
#include <algorithm>
#include <utility>

namespace exchange
{
    OrderBook::OrderBook(std::string symbol)
    : symbol_(std::move(symbol))
    {
    }

    void OrderBook::addOrder(const Order& order)
    {
        if (orders_.find(order.id) != orders_.end())
        {
            throw std::invalid_argument(
                "Order ID already exists in OrderBook"
            );
        }

        if (order.symbol != symbol_)
        {
            throw std::invalid_argument(
                "Order symbol does not match OrderBook symbol"
            );
        }

        if (order.remainingQuantity == 0)
        {
            throw std::invalid_argument(
                "Cannot add an order with zero remaining quantity"
            );
        }

        if (order.side == Side::Buy)
        {
            auto [it, inserted] = bids_.try_emplace(
                order.priceInCents,
                order.priceInCents
            );

            it->second.addOrder(order.id,order.remainingQuantity);
        } 
        else 
        {
            auto [it, inserted] = asks_.try_emplace(
                order.priceInCents,
                order.priceInCents
            );

            it->second.addOrder(order.id,order.remainingQuantity); 
        }
        orders_.emplace(order.id, order);
    }

    bool OrderBook::empty() const
    {
        return bids_.empty() && asks_.empty();
    }

    bool OrderBook::empty(Side side) const
    {
        if (side == Side::Buy)
        {
            return bids_.empty();
        }

        return asks_.empty();
    }

    const PriceLevel* OrderBook::bestBid() const
    {
        if (bids_.empty())
        {
            return nullptr;
        }

        return &bids_.begin()->second;
    }

    const PriceLevel* OrderBook::bestAsk() const
    {
        if (asks_.empty())
        {
            return nullptr;
        }

        return &asks_.begin()->second;
    }

    void OrderBook::cancelOrder(const OrderId& orderId)
    {
        auto orderIt = orders_.find(orderId);

        if (orderIt == orders_.end())
        {
            throw std::invalid_argument(
                "Order ID not found in OrderBook"
            );
        }

        const Order& order = orderIt->second;

        if (order.side == Side::Buy)
        {
            auto levelIt = bids_.find(order.priceInCents);

            if (levelIt == bids_.end())
            {
                throw std::logic_error(
                    "OrderBook is inconsistent: bid PriceLevel not found"
                );
            }

            levelIt->second.removeOrder(orderId);
            levelIt->second.reduceQuantity(order.remainingQuantity);

            if (levelIt->second.empty())
            {
                bids_.erase(levelIt);
            }
        }
        else
        {
            auto levelIt = asks_.find(order.priceInCents);

            if (levelIt == asks_.end())
            {
                throw std::logic_error(
                    "OrderBook is inconsistent: ask PriceLevel not found"
                );
            }

            levelIt->second.removeOrder(orderId);
            levelIt->second.reduceQuantity(order.remainingQuantity);
            if (levelIt->second.empty())
            {
                asks_.erase(levelIt);
            }
        }

        orders_.erase(orderIt);
    }

    Order* OrderBook::findOrder(const OrderId& orderId)
    {
        auto it = orders_.find(orderId);

        if (it == orders_.end())
        {
            return nullptr;
        }

        return &it->second;
    }

    const Order* OrderBook::findOrder( const OrderId& orderId) const
    {
        auto it = orders_.find(orderId);

        if (it == orders_.end())
        {
            return nullptr;
        }

        return &it->second;
    }

    void OrderBook::fillOrder(
        const OrderId& orderId,
        uint64_t quantity
    )
    {
        auto orderIt = orders_.find(orderId);

        if (orderIt == orders_.end())
        {
            throw std::invalid_argument(
                "Order ID not found in OrderBook"
            );
        }

        Order& order = orderIt->second;

        if (quantity > order.remainingQuantity)
        {
            throw std::invalid_argument(
                "Fill quantity exceeds remaining order quantity"
            );
        }

        if (order.side == Side::Buy)
        {
            auto levelIt = bids_.find(order.priceInCents);

            if (levelIt == bids_.end())
            {
                throw std::logic_error(
                    "OrderBook is inconsistent: bid PriceLevel not found"
                );
            }

            order.fill(quantity);

            levelIt->second.reduceQuantity(quantity);

            if (order.remainingQuantity == 0)
            {
                levelIt->second.removeOrder(orderId);
                levelIt->second.reduceQuantity(order.remainingQuantity);

                if (levelIt->second.empty())
                {
                    bids_.erase(levelIt);
                }

                orders_.erase(orderIt);
            }
        }
        else
        {
            auto levelIt = asks_.find(order.priceInCents);

            if (levelIt == asks_.end())
            {
                throw std::logic_error(
                    "OrderBook is inconsistent: ask PriceLevel not found"
                );
            }

            order.fill(quantity);

            levelIt->second.reduceQuantity(quantity);

            if (order.remainingQuantity == 0)
            {
                levelIt->second.removeOrder(orderId);
                levelIt->second.reduceQuantity(order.remainingQuantity);

                if (levelIt->second.empty())
                {
                    asks_.erase(levelIt);
                }

                orders_.erase(orderIt);
            }
        }
    }

    const std::string& OrderBook::symbol() const
    {
        return symbol_;
    }

    void OrderBook::amendOrder(const OrderId& orderId, uint64_t newQuantity, int64_t newPriceInCents)
    {
        auto orderIt = orders_.find(orderId);

        if (orderIt == orders_.end())
        {
            throw std::invalid_argument(
                "Order ID not found in OrderBook"
            );
        }

        if (newQuantity == 0)
        {
            throw std::invalid_argument(
                "Cannot amend an order to zero quantity"
            );
        }

        Order& order = orderIt->second;

        uint64_t oldQuantity = order.remainingQuantity;
        int64_t oldPrice = order.priceInCents;

        // Same price: preserve FIFO position
        if (newPriceInCents == oldPrice)
        {
            if (order.side == Side::Buy)
            {
                auto levelIt = bids_.find(oldPrice);

                if (levelIt == bids_.end())
                {
                    throw std::logic_error(
                        "OrderBook is inconsistent: bid PriceLevel not found"
                    );
                }

                if (newQuantity < oldQuantity)
                {
                    levelIt->second.reduceQuantity(
                        oldQuantity - newQuantity
                    );
                }
                else if (newQuantity > oldQuantity)
                {
                    levelIt->second.addQuantity(
                        newQuantity - oldQuantity
                    );
                }
            }
            else
            {
                auto levelIt = asks_.find(oldPrice);

                if (levelIt == asks_.end())
                {
                    throw std::logic_error(
                        "OrderBook is inconsistent: ask PriceLevel not found"
                    );
                }

                if (newQuantity < oldQuantity)
                {
                    levelIt->second.reduceQuantity(
                        oldQuantity - newQuantity
                    );
                }
                else if (newQuantity > oldQuantity)
                {
                    levelIt->second.addQuantity(
                        newQuantity - oldQuantity
                    );
                }
            }

            order.remainingQuantity = newQuantity;
            return;
        }

        // Different price: lose FIFO priority

        if (order.side == Side::Buy)
        {
            auto levelIt = bids_.find(oldPrice);

            if (levelIt == bids_.end())
            {
                throw std::logic_error(
                    "OrderBook is inconsistent: bid PriceLevel not found"
                );
            }

            levelIt->second.removeOrder(orderId);
            levelIt->second.reduceQuantity(oldQuantity);

            if (levelIt->second.empty())
            {
                bids_.erase(levelIt);
            }

            order.priceInCents = newPriceInCents;
            order.remainingQuantity = newQuantity;

            auto [newLevelIt, inserted] = bids_.try_emplace(
                newPriceInCents,
                newPriceInCents
            );

            newLevelIt->second.addOrder(
                orderId,
                newQuantity
            );
        }
        else
        {
            auto levelIt = asks_.find(oldPrice);

            if (levelIt == asks_.end())
            {
                throw std::logic_error(
                    "OrderBook is inconsistent: ask PriceLevel not found"
                );
            }

            levelIt->second.removeOrder(orderId);
            levelIt->second.reduceQuantity(oldQuantity);

            if (levelIt->second.empty())
            {
                asks_.erase(levelIt);
            }

            order.priceInCents = newPriceInCents;
            order.remainingQuantity = newQuantity;

            auto [newLevelIt, inserted] = asks_.try_emplace(
                newPriceInCents,
                newPriceInCents
            );

            newLevelIt->second.addOrder(
                orderId,
                newQuantity
            );
        }
    }
}