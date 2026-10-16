#pragma once

#include "exchange/Exchange.h"
#include "exchange/ExchangeCommand.h"

#include <string>

namespace exchange
{
    class LoggedExchange
    {
    public:
        LoggedExchange(
            Exchange& exchange,
            std::string logPath
        );

        void execute(const ExchangeCommand& command);

        bool healthy() const noexcept;

    private:
        Exchange& exchange_;
        std::string logPath_;
        bool healthy_{true};
    };
}