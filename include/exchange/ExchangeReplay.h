#pragma once

#include "exchange/Exchange.h"
#include "exchange/ExchangeCommand.h"

#include <vector>

namespace exchange
{
    class ExchangeReplay
    {
    public:
        static void apply(
            Exchange& exchange,
            const ExchangeCommand& command
        );

        static void replay(
            Exchange& exchange,
            const std::vector<ExchangeCommand>& commands
        );
    };
}