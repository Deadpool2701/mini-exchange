#include "exchange/ExchangeReplay.h"

#include <type_traits>
#include <utility>

namespace exchange
{
    void ExchangeReplay::apply( Exchange& exchange, const ExchangeCommand& command )
    {
        std::visit(
            [&exchange](const auto& cmd)
            {
                using CommandType = std::decay_t<decltype(cmd)>;

                if constexpr (
                    std::is_same_v<CommandType, AddSymbolCommand>
                )
                {
                    exchange.addSymbol(cmd.symbol);
                }
                else if constexpr (
                    std::is_same_v<CommandType, SubmitOrderCommand>
                )
                {
                    exchange.submitOrder(cmd.order);
                }
                else if constexpr (
                    std::is_same_v<CommandType, CancelOrderCommand>
                )
                {
                    exchange.cancelOrder(cmd.orderId);
                }
                else if constexpr (
                    std::is_same_v<CommandType, AmendOrderCommand>
                )
                {
                    exchange.amendOrder(
                        cmd.orderId,
                        cmd.newQuantity,
                        cmd.newPriceInCents
                    );
                }
            },
            command
        );
    }

    void ExchangeReplay::replay(
        Exchange& exchange,
        const std::vector<ExchangeCommand>& commands
    )
    {
        for (const auto& command : commands)
        {
            apply(exchange, command);
        }
    }
}