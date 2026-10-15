#pragma once

#include "exchange/ExchangeCommand.h"

#include <string>
#include <vector>

namespace exchange
{
    class ExchangeCommandLog
    {
    public:
        static void write(
            const std::string& path,
            const std::vector<ExchangeCommand>& commands
        );

        static std::vector<ExchangeCommand> read(
            const std::string& path
        );
    };
}