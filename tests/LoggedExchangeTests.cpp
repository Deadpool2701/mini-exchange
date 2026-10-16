#include "exchange/ExchangeCommandLog.h"
#include "exchange/LoggedExchange.h"
#include "exchange/ExchangeReplay.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <system_error>
#include <variant>
#include <cstddef>

namespace
{
    class TemporaryLog
    {
    public:
        explicit TemporaryLog(const std::string& name)
            : path_(
                  std::filesystem::temp_directory_path() /
                  ("mini_exchange_logged_" + name + ".log")
              )
        {
            std::error_code error;
            std::filesystem::remove(path_, error);
        }

        ~TemporaryLog()
        {
            std::error_code error;
            std::filesystem::remove(path_, error);
        }

        std::string path() const
        {
            return path_.string();
        }

    private:
        std::filesystem::path path_;
    };

    exchange::Order makeOrder(
        std::uint64_t id,
        const std::string& symbol
    )
    {
        return exchange::Order{
            exchange::OrderId{id},
            symbol,
            exchange::Side::Buy,
            exchange::OrderType::Limit,
            10,
            10,
            18550
        };
    }
}

TEST(
    LoggedExchangeTest,
    PersistsSuccessfullyExecutedCommand
)
{
    TemporaryLog log("successful_execution");

    exchange::Exchange underlying;
    exchange::LoggedExchange logged(underlying, log.path());

    logged.execute(exchange::AddSymbolCommand{"AAPL"});

    EXPECT_TRUE(logged.healthy());

    const auto commands =
        exchange::ExchangeCommandLog::read(log.path());

    ASSERT_EQ(commands.size(), 1U);
    EXPECT_TRUE(
        std::holds_alternative<exchange::AddSymbolCommand>(
            commands[0]
        )
    );
}

TEST(
    LoggedExchangeTest,
    DoesNotPersistRejectedCommand
)
{
    TemporaryLog log("rejected_execution");

    exchange::Exchange underlying;
    exchange::LoggedExchange logged(underlying, log.path());

    logged.execute(exchange::AddSymbolCommand{"AAPL"});

    // MSFT has not been registered with this exchange.
    const exchange::ExchangeCommand rejected{
        exchange::SubmitOrderCommand{
            makeOrder(1701, "MSFT")
        }
    };

    EXPECT_ANY_THROW(logged.execute(rejected));

    // A rejected command must not be appended to the log.
    const auto commands =
        exchange::ExchangeCommandLog::read(log.path());

    ASSERT_EQ(commands.size(), 1U);
    EXPECT_TRUE(
        std::holds_alternative<exchange::AddSymbolCommand>(
            commands[0]
        )
    );

    EXPECT_TRUE(logged.healthy());
}

TEST(
    LoggedExchangeTest,
    AppendFailureMarksWrapperUnhealthy
)
{
    const auto missingDirectory =
        std::filesystem::temp_directory_path() /
        "mini_exchange_logged_missing_parent";

    std::error_code error;
    std::filesystem::remove_all(missingDirectory, error);

    const auto logPath =
        (missingDirectory / "commands.log").string();

    exchange::Exchange underlying;
    exchange::LoggedExchange logged(underlying, logPath);

    EXPECT_ANY_THROW(
        logged.execute(exchange::AddSymbolCommand{"AAPL"})
    );

    EXPECT_FALSE(logged.healthy());
}

TEST(
    LoggedExchangeTest,
    UnhealthyWrapperRejectsSubsequentCommands
)
{
    const auto missingDirectory =
        std::filesystem::temp_directory_path() /
        "mini_exchange_logged_unhealthy_parent";

    std::error_code error;
    std::filesystem::remove_all(missingDirectory, error);

    const auto logPath =
        (missingDirectory / "commands.log").string();

    exchange::Exchange underlying;
    exchange::LoggedExchange logged(underlying, logPath);

    // The exchange operation succeeds, but persistence fails.
    EXPECT_ANY_THROW(
        logged.execute(exchange::AddSymbolCommand{"AAPL"})
    );

    ASSERT_FALSE(logged.healthy());

    // The wrapper must reject further operations.
    EXPECT_THROW(
        logged.execute(exchange::AddSymbolCommand{"MSFT"}),
        std::logic_error
    );

    EXPECT_FALSE(logged.healthy());
}

TEST(
    LoggedExchangeTest,
    ReconstructedExchangeMatchesLiveExchange
)
{
    TemporaryLog log("recovery_equivalence");

    exchange::Exchange liveExchange;
    exchange::LoggedExchange logged(
        liveExchange,
        log.path()
    );

    // 1. Register the symbol.
    logged.execute(
        exchange::AddSymbolCommand{"AAPL"}
    );

    // 2. Rest a sell order.
    logged.execute(
        exchange::SubmitOrderCommand{
            exchange::Order{
                exchange::OrderId{1701},
                "AAPL",
                exchange::Side::Sell,
                exchange::OrderType::Limit,
                100,
                100,
                18550
            }
        }
    );

    // 3. Submit a matching buy order.
    logged.execute(
        exchange::SubmitOrderCommand{
            exchange::Order{
                exchange::OrderId{1702},
                "AAPL",
                exchange::Side::Buy,
                exchange::OrderType::Limit,
                40,
                40,
                18550
            }
        }
    );

    // 4. Amend the remaining active order.
    logged.execute(
        exchange::AmendOrderCommand{
            exchange::OrderId{1701},
            80,
            18600
        }
    );

    // 5. Submit another resting order, then cancel it.
    logged.execute(
        exchange::SubmitOrderCommand{
            exchange::Order{
                exchange::OrderId{1703},
                "AAPL",
                exchange::Side::Sell,
                exchange::OrderType::Limit,
                25,
                25,
                18700
            }
        }
    );

    logged.execute(
        exchange::CancelOrderCommand{
            exchange::OrderId{1703}
        }
    );

    ASSERT_TRUE(logged.healthy());

    // Reconstruct a completely separate exchange from disk.
    const auto commands =
        exchange::ExchangeCommandLog::read(log.path());

    exchange::Exchange recoveredExchange;

    exchange::ExchangeReplay::replay(
        recoveredExchange,
        commands
    );

    // Compare active-order presence.
    const auto liveActive =
        liveExchange.findActiveOrder(exchange::OrderId{1701});

    const auto recoveredActive =
        recoveredExchange.findActiveOrder(
            exchange::OrderId{1701}
        );

    ASSERT_TRUE(liveActive.has_value());
    ASSERT_TRUE(recoveredActive.has_value());

    // Compare the active order's state.
    EXPECT_EQ(
        recoveredActive->id.value,
        liveActive->id.value
    );
    EXPECT_EQ(recoveredActive->symbol, liveActive->symbol);
    EXPECT_EQ(recoveredActive->side, liveActive->side);
    EXPECT_EQ(recoveredActive->type, liveActive->type);
    EXPECT_EQ(recoveredActive->quantity, liveActive->quantity);
    EXPECT_EQ(
        recoveredActive->remainingQuantity,
        liveActive->remainingQuantity
    );
    EXPECT_EQ(
        recoveredActive->priceInCents,
        liveActive->priceInCents
    );

    // The fully filled order and cancelled order must not be active.
    EXPECT_FALSE(
        liveExchange.findActiveOrder(
            exchange::OrderId{1702}
        ).has_value()
    );

    EXPECT_FALSE(
        recoveredExchange.findActiveOrder(
            exchange::OrderId{1702}
        ).has_value()
    );

    EXPECT_FALSE(
        liveExchange.findActiveOrder(
            exchange::OrderId{1703}
        ).has_value()
    );

    EXPECT_FALSE(
        recoveredExchange.findActiveOrder(
            exchange::OrderId{1703}
        ).has_value()
    );

    // Compare trade history.
    const auto& liveTrades =
        liveExchange.tradesForSymbol("AAPL");

    const auto& recoveredTrades =
        recoveredExchange.tradesForSymbol("AAPL");

    ASSERT_EQ(recoveredTrades.size(), liveTrades.size());

    for (std::size_t i = 0; i < liveTrades.size(); ++i)
    {
        EXPECT_EQ(
            recoveredTrades[i].buyOrderId.value,
            liveTrades[i].buyOrderId.value
        );

        EXPECT_EQ(
            recoveredTrades[i].sellOrderId.value,
            liveTrades[i].sellOrderId.value
        );

        EXPECT_EQ(
            recoveredTrades[i].quantity,
            liveTrades[i].quantity
        );

        EXPECT_EQ(
            recoveredTrades[i].priceInCents,
            liveTrades[i].priceInCents
        );
    }
}

