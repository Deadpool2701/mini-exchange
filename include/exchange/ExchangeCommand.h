#pragma once

#include "exchange/Order.h"
#include "exchange/OrderId.h"

#include <cstdint>
#include <string>
#include <variant>

namespace exchange
{
    struct AddSymbolCommand
    {
        std::string symbol;
    };

    struct SubmitOrderCommand
    {
        Order order;
    };

    struct CancelOrderCommand
    {
        OrderId orderId;
    };

    struct AmendOrderCommand
    {
        OrderId orderId;
        uint64_t newQuantity;
        int64_t newPriceInCents;
    };

    using ExchangeCommand = std::variant<
        AddSymbolCommand,
        SubmitOrderCommand,
        CancelOrderCommand,
        AmendOrderCommand
    >;
}