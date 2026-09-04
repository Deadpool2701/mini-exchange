#include "exchange/MatchingEngine.h"

#include <utility>
#include <algorithm>

namespace exchange
{
    MatchingEngine::MatchingEngine(std::string symbol)
        : orderBook_(std::move(symbol))
    {
    }

    const OrderBook& MatchingEngine::orderBook() const
    {
        return orderBook_;
    }

    std::vector<Trade> MatchingEngine::submitOrder(Order order)
    {
        std::vector<Trade> trades;
        if (order.symbol != orderBook_.symbol())
        {
            throw std::invalid_argument(
                "Order symbol does not match MatchingEngine symbol"
            );
        }

        if (order.side == Side::Buy)
        {
            while (order.remainingQuantity > 0 && orderBook_.bestAsk() != nullptr &&
                   order.priceInCents >= orderBook_.bestAsk()->price())
            {
                auto bestAsk = orderBook_.bestAsk();
                auto bestAskOrderId = bestAsk->frontOrder();
                const Order* bestAskOrder = orderBook_.findOrder(bestAskOrderId);

                if (bestAskOrder == nullptr)
                {
                    throw std::logic_error(
                        "OrderBook is inconsistent: ask order not found"
                    );
                }

                uint64_t fillQuantity = std::min(order.remainingQuantity, bestAskOrder->remainingQuantity);

                Trade trade{
                    order.id,
                    bestAskOrderId,
                    fillQuantity,
                    bestAsk->price()
                };

                trades.push_back(trade);
                trades_.push_back(trade);

                orderBook_.fillOrder(bestAskOrderId, fillQuantity);
                order.fill(fillQuantity);
            }
        }
        else
        {
            while (order.remainingQuantity > 0 && orderBook_.bestBid() != nullptr &&
                   order.priceInCents <= orderBook_.bestBid()->price())
            {
                auto bestBid = orderBook_.bestBid();
                auto bestBidOrderId = bestBid->frontOrder();
                const Order* bestBidOrder = orderBook_.findOrder(bestBidOrderId);

                if (bestBidOrder == nullptr)
                {
                    throw std::logic_error(
                        "OrderBook is inconsistent: bid order not found"
                    );
                }

                uint64_t fillQuantity = std::min(order.remainingQuantity, bestBidOrder->remainingQuantity);

                Trade trade{
                    bestBidOrderId,
                    order.id,
                    fillQuantity,
                    bestBid->price()
                };

                trades.push_back(trade);
                trades_.push_back(trade);

                orderBook_.fillOrder(bestBidOrderId, fillQuantity);
                order.fill(fillQuantity);
            }
        }

        if (order.remainingQuantity > 0)
        {
            orderBook_.addOrder(order);
        }

        return trades;
    }

    void MatchingEngine::cancelOrder(const OrderId& orderId)
    {
        orderBook_.cancelOrder(orderId);
    }

    void MatchingEngine::amendOrder(const OrderId& orderId, uint64_t newQuantity, int64_t newPriceInCents)
    {
        orderBook_.amendOrder(orderId, newQuantity, newPriceInCents);
    }

    const std::vector<Trade>& MatchingEngine::trades() const
    {
        return trades_;
    }

} // namespace exchange