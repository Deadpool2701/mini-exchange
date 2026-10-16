#include "exchange/LoggedExchange.h"

#include "exchange/ExchangeCommandLog.h"
#include "exchange/ExchangeReplay.h"

#include <stdexcept>
#include <utility>

namespace exchange
{
    LoggedExchange::LoggedExchange(
        Exchange& exchange,
        std::string logPath
    )
        : exchange_(exchange),
          logPath_(std::move(logPath))
    {
        if (logPath_.empty())
        {
            throw std::invalid_argument(
                "Command log path cannot be empty"
            );
        }
    }

    void LoggedExchange::execute(
        const ExchangeCommand& command
    )
    {
        if (!healthy_)
        {
            throw std::logic_error(
                "LoggedExchange is unhealthy; recovery is required"
            );
        }

        // Apply the command using the existing exchange logic.
        ExchangeReplay::apply(exchange_, command);

        // Persist it only after execution completes successfully.
        try
        {
            ExchangeCommandLog::append(logPath_, command);
        }
        catch (...)
        {
            // Exchange state and persisted history may now differ.
            // Do not allow further commands through this wrapper.
            healthy_ = false;
            throw;
        }
    }

    bool LoggedExchange::healthy() const noexcept
    {
        return healthy_;
    }
}