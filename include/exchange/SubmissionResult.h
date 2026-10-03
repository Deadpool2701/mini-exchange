#pragma once

#include <vector>

#include <exchange/OrderEvent.h>
#include <exchange/Trade.h>

namespace exchange
{
    struct SubmissionResult
    {
        std::vector<OrderEvent> events;
        std::vector<Trade> trades;
    };
}