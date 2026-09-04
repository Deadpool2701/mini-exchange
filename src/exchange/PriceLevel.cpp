#include "exchange/PriceLevel.h"
#include <stdexcept>
#include <algorithm>

namespace exchange
{
    PriceLevel::PriceLevel(int64_t priceInCents)
        : priceInCents_(priceInCents),
        totalRemainingQuantity_(0)
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
        orders_.push_back(orderId);
        totalRemainingQuantity_ += quantity;
    }

    void PriceLevel::removeOrder(const OrderId& orderId)
    {
        auto it = std::find(
            orders_.begin(),
            orders_.end(),
            orderId
        );

        if (it == orders_.end())
        {
            throw std::invalid_argument(
                "OrderId not found in PriceLevel"
            );
        }

        orders_.erase(it);
    }

    int64_t PriceLevel::price() const
    {
        return priceInCents_;
    }

    uint64_t PriceLevel::totalQuantity() const
    {
        return totalRemainingQuantity_;
    }

    bool PriceLevel::empty() const
    {
        return orders_.empty();
    }

    const OrderId& PriceLevel::frontOrder() const
    {
        if (orders_.empty())
        {
            throw std::out_of_range("No orders in PriceLevel");
        }
        return orders_.front();
    }

    void PriceLevel::reduceQuantity(uint64_t quantity)
    {
        if (quantity > totalRemainingQuantity_)
        {
            throw std::invalid_argument(
                "Reduction quantity exceeds total remaining quantity"
            );
        }

        totalRemainingQuantity_ -= quantity;
    }

    void PriceLevel::addQuantity(uint64_t quantity)
    {
        if (quantity == 0)
        {
            throw std::invalid_argument(
                "Cannot add zero quantity"
            );
        }

        totalRemainingQuantity_ += quantity;
    }

} // namespace exchange