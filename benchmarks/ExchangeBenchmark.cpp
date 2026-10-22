#include "exchange/Exchange.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>
#include <stdexcept>

namespace {

constexpr std::size_t OrderCount = 100'000;
constexpr std::size_t PriceLevelCount = 100;
constexpr std::size_t MatchingOrderCount = 10'000;
constexpr std::size_t Repetitions = 5;
constexpr std::size_t CancellationOrderCount = 10'000;
constexpr std::size_t MultiSymbolOrderCount = 2'000;
constexpr std::size_t MultiSymbolMatchCount = 1'000;
constexpr std::size_t MultiSymbolSymbolCount = 3;


constexpr std::int64_t StartingPrice = 10'000;

double runOrderSubmissionBenchmark()
{
    exchange::Exchange exchange;
    exchange.addSymbol("BENCH");

    const auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < OrderCount; ++i)
    {
        const auto orderId = static_cast<std::uint64_t>(i + 1);

        const auto price =
            StartingPrice +
            static_cast<std::int64_t>(i % PriceLevelCount);

        exchange::Order order{
            exchange::OrderId{orderId},
            "BENCH",
            exchange::Side::Buy,
            exchange::OrderType::Limit,
            100,
            100,
            price
        };

        exchange.submitOrder(order);
    }

    const auto end = std::chrono::steady_clock::now();

    return std::chrono::duration<double>(end - start).count();
}

double runMatchingBenchmark()
{
    exchange::Exchange exchange;
    exchange.addSymbol("BENCH");

    // Create resting sell liquidity.
    for (std::size_t i = 0; i < MatchingOrderCount; ++i)
    {
        const auto orderId =
            static_cast<std::uint64_t>(i + 1);

        exchange::Order order{
            exchange::OrderId{orderId},
            "BENCH",
            exchange::Side::Sell,
            exchange::OrderType::Limit,
            100,
            100,
            StartingPrice
        };

        exchange.submitOrder(order);
    }

    const auto start = std::chrono::steady_clock::now();

    // Aggressively cross the resting sell orders.
    for (std::size_t i = 0; i < MatchingOrderCount; ++i)
    {
        const auto orderId =
            static_cast<std::uint64_t>(
                MatchingOrderCount + i + 1);

        exchange::Order order{
            exchange::OrderId{orderId},
            "BENCH",
            exchange::Side::Buy,
            exchange::OrderType::Limit,
            100,
            100,
            StartingPrice
        };

        exchange.submitOrder(order);
    }

    const auto end = std::chrono::steady_clock::now();

    return std::chrono::duration<double>(end - start).count();
}

void printBenchmark(
    const std::string& name,
    const std::vector<double>& times,
    std::size_t operationCount)
{
    std::vector<double> sortedTimes = times;
    std::sort(sortedTimes.begin(), sortedTimes.end());

    const double minimum = sortedTimes.front();
    const double median = sortedTimes[sortedTimes.size() / 2];

    double total = 0.0;

    for (const double time : times)
    {
        total += time;
    }

    const double average = total / times.size();

    const double throughput =
        static_cast<double>(operationCount) / median;

    std::cout << name << '\n';
    std::cout << "------------------\n";
    std::cout << "Operations:        "
              << operationCount << '\n';
    std::cout << "Repetitions:       "
              << times.size() << '\n';
    std::cout << "Minimum:           "
              << minimum << " seconds\n";
    std::cout << "Median:            "
              << median << " seconds\n";
    std::cout << "Average:           "
              << average << " seconds\n";
    std::cout << "Median throughput: "
              << throughput << " orders/sec\n\n";
}

double runCancellationBenchmark()
{
    exchange::Exchange exchange;
    exchange.addSymbol("BENCH");

    std::vector<std::uint64_t> orderIds;
    orderIds.reserve(CancellationOrderCount);

    // Create resting orders outside the timed section.
    for (std::size_t i = 0; i < CancellationOrderCount; ++i)
    {
        const auto orderId =
            static_cast<std::uint64_t>(i + 1);

        const auto price =
            StartingPrice +
            static_cast<std::int64_t>(i % PriceLevelCount);

        exchange::Order order{
            exchange::OrderId{orderId},
            "BENCH",
            exchange::Side::Buy,
            exchange::OrderType::Limit,
            100,
            100,
            price
        };

        exchange.submitOrder(order);
        orderIds.push_back(orderId);
    }

    const auto start = std::chrono::steady_clock::now();

    for (const auto orderId : orderIds)
    {
        exchange.cancelOrder(exchange::OrderId{orderId});
    }

    const auto end = std::chrono::steady_clock::now();

    // Correctness checks happen outside the timed section.
    for (const auto orderId : orderIds)
    {
        if (exchange.findActiveOrder(exchange::OrderId{orderId}).has_value())
        {
            throw std::logic_error(
                "Cancellation benchmark: order is still active"
            );
        }
    }

    return std::chrono::duration<double>(end - start).count();
}

double runMultiSymbolBenchmark()
{
    exchange::Exchange exchange;

    const std::vector<std::string> symbols{
        "BENCH_A",
        "BENCH_B",
        "BENCH_C"
    };

    for (const auto& symbol : symbols)
    {
        exchange.addSymbol(symbol);
    }

    std::vector<std::vector<exchange::OrderId>> restingOrderIds;
    restingOrderIds.resize(symbols.size());

    std::uint64_t nextOrderId = 1;

    // Create resting sell liquidity for every symbol.
    // This setup is outside the timed section.
    for (std::size_t symbolIndex = 0;
         symbolIndex < symbols.size();
         ++symbolIndex)
    {
        auto& ids = restingOrderIds[symbolIndex];
        ids.reserve(MultiSymbolOrderCount);

        for (std::size_t i = 0;
             i < MultiSymbolOrderCount;
             ++i)
        {
            const auto orderId = nextOrderId++;

            exchange::Order order{
                exchange::OrderId{orderId},
                symbols[symbolIndex],
                exchange::Side::Sell,
                exchange::OrderType::Limit,
                100,
                100,
                StartingPrice
            };

            exchange.submitOrder(order);
            ids.push_back(order.id);
        }
    }

    const auto start = std::chrono::steady_clock::now();

    for (std::size_t symbolIndex = 0;
         symbolIndex < symbols.size();
         ++symbolIndex)
    {
        const auto& symbol = symbols[symbolIndex];
        const auto& ids = restingOrderIds[symbolIndex];

        // Match half of the resting orders.
        for (std::size_t i = 0;
             i < MultiSymbolMatchCount;
             ++i)
        {
            const auto orderId = nextOrderId++;

            exchange::Order order{
                exchange::OrderId{orderId},
                symbol,
                exchange::Side::Buy,
                exchange::OrderType::Limit,
                100,
                100,
                StartingPrice
            };

            exchange.submitOrder(order);
        }

        // Cancel the other half.
        for (std::size_t i = MultiSymbolMatchCount;
             i < MultiSymbolOrderCount;
             ++i)
        {
            exchange.cancelOrder(ids[i]);
        }
    }

    const auto end = std::chrono::steady_clock::now();

    // Correctness checks are outside the timed section.
    for (const auto& symbol : symbols)
    {
        const auto& engine = exchange.matchingEngine(symbol);

        if (!engine.orderBook().empty())
        {
            throw std::logic_error(
                "Multi-symbol benchmark: OrderBook is not empty"
            );
        }

        const auto& trades = exchange.tradesForSymbol(symbol);

        for (const auto& trade : trades)
        {
            if (trade.quantity != 100 ||
                trade.priceInCents != StartingPrice)
            {
                throw std::logic_error(
                    "Multi-symbol benchmark: unexpected trade"
                );
            }
        }

        if (trades.size() != MultiSymbolMatchCount)
        {
            throw std::logic_error(
                "Multi-symbol benchmark: unexpected trade count"
            );
        }
    }

    return std::chrono::duration<double>(end - start).count();
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr
            << "Usage: exchange_benchmark "
            << "<submission|matching|cancellation|multi-symbol>\n";
        return 1;
    }

    const std::string workload = argv[1];

    if (workload == "submission")
    {
        std::vector<double> times;
        times.reserve(Repetitions);

        for (std::size_t i = 0; i < Repetitions; ++i)
        {
            times.push_back(
                runOrderSubmissionBenchmark());
        }

        printBenchmark(
            "Order Submission Benchmark",
            times,
            OrderCount);

        return 0;
    }

    if (workload == "matching")
    {
        std::vector<double> times;
        times.reserve(Repetitions);

        for (std::size_t i = 0; i < Repetitions; ++i)
        {
            times.push_back(
                runMatchingBenchmark());
        }

        printBenchmark(
            "Matching Benchmark",
            times,
            MatchingOrderCount);

        return 0;
    }

    if (workload == "cancellation")
    {
        std::vector<double> times;
        times.reserve(Repetitions);

        for (std::size_t i = 0; i < Repetitions; ++i)
        {
            times.push_back(
                runCancellationBenchmark());
        }

        printBenchmark(
            "Cancellation Benchmark",
            times,
            CancellationOrderCount);

        return 0;
    }

    if (workload == "multi-symbol")
    {
        std::vector<double> times;
        times.reserve(Repetitions);

        for (std::size_t i = 0; i < Repetitions; ++i)
        {
            times.push_back(
                runMultiSymbolBenchmark());
        }

        printBenchmark(
            "Multi-Symbol Mixed Benchmark",
            times,
            MultiSymbolSymbolCount *
                (MultiSymbolMatchCount +
                 (MultiSymbolOrderCount -
                  MultiSymbolMatchCount)));

        return 0;
    }

    std::cerr
        << "Unknown workload: "
        << workload << '\n';

    return 1;
}