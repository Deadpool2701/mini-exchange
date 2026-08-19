#include "exchange/PriceLevel.h"
#include <stdexcept>
#include <algorithm>

namespace exchange
{
    PriceLevel::PriceLevel(int64_t priceInCents)
        : priceInCents_(priceInCents)
    {
    }

    void PriceLevel::addOrder(const OrderId& orderId, uint64_t quantity)
    {
        if (quantity == 0)
        {
            throw std::invalid_argument(
                "Cannot add an order with zero quantity"
            );
        }
        orders_.push_back(OrderEntry{orderId, quantity});
        totalQuantity_ += quantity;
    }

    void PriceLevel::removeOrder(const OrderId& orderId)
    {
        auto it = std::find_if(
            orders_.begin(),
            orders_.end(),
            [&orderId](const OrderEntry& entry)
            {
                return entry.id == orderId;
            }
        );

        if(it == orders_.end())
        {
            throw std::invalid_argument("OrderId not found in PriceLevel");
        }
        totalQuantity_ -= it->remainingQuantity;
        orders_.erase(it);
    }

    int64_t PriceLevel::price() const
    {
        return priceInCents_;
    }

    uint64_t PriceLevel::totalQuantity() const
    {
        return totalQuantity_;
    }

    void PriceLevel::fill(uint64_t quantity)
    {
        if (quantity > totalQuantity_)
        {
            throw std::invalid_argument("Fill quantity exceeds total quantity at this Price Level");
        }

        uint64_t remainingToFill = quantity;
        while (remainingToFill > 0)
        {
            OrderEntry& frontOrder = orders_.front();
            if (frontOrder.remainingQuantity <= remainingToFill)
            {
                remainingToFill -= frontOrder.remainingQuantity;
                totalQuantity_ -= frontOrder.remainingQuantity;
                orders_.pop_front();
            }
            else
            {
                frontOrder.remainingQuantity -= remainingToFill;
                totalQuantity_ -= remainingToFill;
                remainingToFill = 0;
            }
        }
    }

    bool PriceLevel::empty() const
    {
        return orders_.empty();
    }

    const OrderEntry& PriceLevel::frontOrder() const
    {
        if (orders_.empty())
        {
            throw std::out_of_range("No orders in PriceLevel");
        }
        return orders_.front();
    }

} // namespace exchange