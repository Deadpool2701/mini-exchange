#include <exchange/Exchange.h>

#include <stdexcept>
#include <utility>

namespace exchange
{
    void Exchange::addSymbol(std::string symbol)
    {
        if (symbol.empty())
        {
            throw std::invalid_argument(
                "Cannot add an empty symbol"
            );
        }

        if (hasSymbol(symbol))
        {
            throw std::invalid_argument(
                "Symbol already exists in Exchange"
            );
        }

        engines_.try_emplace(symbol, symbol);
    }

    bool Exchange::hasSymbol(const std::string& symbol) const
    {
        return engines_.find(symbol) != engines_.end();
    }

    const MatchingEngine& Exchange::matchingEngine(const std::string& symbol) const
    {
        auto it = engines_.find(symbol);

        if (it == engines_.end())
        {
            throw std::invalid_argument(
                "Symbol not found in Exchange"
            );
        }

        return it->second;
    }

    OrderResult Exchange::submitOrder(Order order)
    {

        if (usedOrderIds_.find(order.id) != usedOrderIds_.end())
        {
            throw std::invalid_argument(
                "Order ID has already been used in Exchange"
            );
        }
        if (orderSymbols_.find(order.id) != orderSymbols_.end())
        {
            throw std::invalid_argument(
                "Order ID already exists in Exchange"
            );
        }

        auto engineIt = engines_.find(order.symbol);

        if (engineIt == engines_.end())
        {
            throw std::invalid_argument(
                "Order symbol is not registered in Exchange"
            );
        }

        MatchingEngine& engine = engineIt->second;

        OrderResult result = engine.submitOrder(order);

        usedOrderIds_.insert(order.id);

        for (const Trade& trade : result.trades)
        {
            const Order* buyOrder = engine.orderBook().findOrder(trade.buyOrderId);

            if (buyOrder != nullptr)
            {
                orderSymbols_[trade.buyOrderId] = order.symbol;
            }
            else
            {
                orderSymbols_.erase(trade.buyOrderId);
            }

            const Order* sellOrder = engine.orderBook().findOrder(trade.sellOrderId);

            if (sellOrder != nullptr)
            {
                orderSymbols_[trade.sellOrderId] = order.symbol;
            }
            else
            {
                orderSymbols_.erase(trade.sellOrderId);
            }
        }

        const Order* incomingOrder = engine.orderBook().findOrder(order.id);

        if (incomingOrder != nullptr)
        {
            orderSymbols_[order.id] = order.symbol;
        }

        return result;
    }

    OrderResult Exchange::cancelOrder(const OrderId& orderId)
    {
        auto symbolIt = orderSymbols_.find(orderId);

        if (symbolIt == orderSymbols_.end())
        {
            throw std::invalid_argument(
                "Cannot cancel order: order is not active in Exchange"
            );
        }

        auto engineIt = engines_.find(symbolIt->second);

        if (engineIt == engines_.end())
        {
            throw std::logic_error(
                "Exchange is inconsistent: order symbol has no MatchingEngine"
            );
        }

        OrderResult result = engineIt->second.cancelOrder(orderId);

        orderSymbols_.erase(symbolIt);

        return result;
    }

    OrderResult Exchange::amendOrder(
        const OrderId& orderId,
        uint64_t newQuantity,
        int64_t newPriceInCents
    )
    {
        auto symbolIt = orderSymbols_.find(orderId);

        if (symbolIt == orderSymbols_.end())
        {
            throw std::invalid_argument(
                "Cannot amend order: order is not active in Exchange"
            );
        }

        auto engineIt = engines_.find(symbolIt->second);

        if (engineIt == engines_.end())
        {
            throw std::logic_error(
                "Exchange is inconsistent: order symbol has no MatchingEngine"
            );
        }

        return engineIt->second.amendOrder(
            orderId,
            newQuantity,
            newPriceInCents
        );
    }

    std::optional<Order> Exchange::findActiveOrder( const OrderId& orderId ) const
    {
        auto symbolIt = orderSymbols_.find(orderId);

        if (symbolIt == orderSymbols_.end())
        {
            return std::nullopt;
        }

        auto engineIt = engines_.find(symbolIt->second);

        if (engineIt == engines_.end())
        {
            throw std::logic_error(
                "Exchange is inconsistent: active order has no MatchingEngine"
            );
        }

        const Order* order = engineIt->second.orderBook().findOrder(orderId);

        if (order == nullptr)
        {
            throw std::logic_error(
                "Exchange is inconsistent: active order is missing from OrderBook"
            );
        }

        return *order;
    }

    const std::vector<Trade>& Exchange::tradesForSymbol( const std::string& symbol ) const
    {
        return matchingEngine(symbol).trades();
    }
} // namespace exchange