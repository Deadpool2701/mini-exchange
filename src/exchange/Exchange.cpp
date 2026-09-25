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

    std::vector<Trade> Exchange::submitOrder(Order order)
    {
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

        std::vector<Trade> trades = engine.submitOrder(order);

        for (const Trade& trade : trades)
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

        return trades;
    }

    void Exchange::cancelOrder(const OrderId& orderId)
    {
        std::string symbol = orderSymbols_.at(orderId);
        auto& engine = engines_.at(symbol);
        engine.cancelOrder(orderId);
        orderSymbols_.erase(orderId);
    }

    void Exchange::amendOrder(const OrderId& orderId, uint64_t newQuantity, int64_t newPriceInCents)
    {
        std::string symbol = orderSymbols_.at(orderId);
        auto& engine = engines_.at(symbol);
        engine.amendOrder(orderId, newQuantity, newPriceInCents);
    }
} // namespace exchange