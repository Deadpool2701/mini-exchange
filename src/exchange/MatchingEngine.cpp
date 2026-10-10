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

    OrderResult MatchingEngine::submitOrder(Order order)
    {
        if (usedOrderIds_.find(order.id) != usedOrderIds_.end())
        {
            throw std::invalid_argument(
                "Order ID has already been used in MatchingEngine"
            );
        }

        if (order.type == OrderType::Market)
        {
            throw std::invalid_argument(
                "Market orders are not supported yet"
            );
        }
        OrderResult result;
        if (order.symbol != orderBook_.symbol())
        {
            throw std::invalid_argument(
                "Order symbol does not match MatchingEngine symbol"
            );
        }

        result.events.push_back(OrderEvent{
            OrderEventType::Accepted,
            order.id,
            order.remainingQuantity,
            order.priceInCents
        });

        usedOrderIds_.insert(order.id);

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

                result.trades.push_back(trade);
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

                result.trades.push_back(trade);
                trades_.push_back(trade);

                orderBook_.fillOrder(bestBidOrderId, fillQuantity);
                order.fill(fillQuantity);
            }
        }

        if (order.remainingQuantity > 0)
        {
            orderBook_.addOrder(order);
        }

        return result;
    }

    OrderResult MatchingEngine::cancelOrder(const OrderId& orderId)
    {
        OrderResult result;
        const Order* order = orderBook_.findOrder(orderId);

        if (order == nullptr)
        {
            throw std::invalid_argument("Order ID not found in OrderBook");
        }

        result.events.push_back(OrderEvent{OrderEventType::Cancelled, order->id, 0, order->priceInCents});
        orderBook_.cancelOrder(orderId);
        return result;
    }

    OrderResult MatchingEngine::amendOrder(const OrderId& orderId, uint64_t newQuantity, int64_t newPriceInCents)
    {
        OrderResult result;
        const Order* order = orderBook_.findOrder(orderId);
        if (order == nullptr)
        {
            throw std::invalid_argument("Order ID not found in OrderBook");
        }

        orderBook_.amendOrder(orderId, newQuantity, newPriceInCents);

        const Order* amendedOrder = orderBook_.findOrder(orderId);
        if (amendedOrder == nullptr)
        {
            throw std::logic_error("OrderBook is inconsistent: amended order not found");
        }

        result.events.push_back(OrderEvent{ 
            OrderEventType::Amended, 
            amendedOrder->id,
            amendedOrder->remainingQuantity,
            amendedOrder->priceInCents
        });

        return result;
    }

    const std::vector<Trade>& MatchingEngine::trades() const
    {
        return trades_;
    }

} // namespace exchange