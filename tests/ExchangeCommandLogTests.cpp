#include <gtest/gtest.h>

#include "exchange/ExchangeCommandLog.h"
#include "exchange/ExchangeReplay.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    class TemporaryLog
    {
    public:
        TemporaryLog()
            : path_(
                std::filesystem::temp_directory_path()
                / "mini_exchange_command_log_test.txt"
            )
        {
            std::filesystem::remove(path_);
        }

        ~TemporaryLog()
        {
            std::error_code error;
            std::filesystem::remove(path_, error);
        }

        const std::string path() const
        {
            return path_.string();
        }

    private:
        std::filesystem::path path_;
    };
}

TEST(ExchangeCommandLogTest, WritesReadsAndReplaysCommands)
{
    TemporaryLog file;

    const std::vector<exchange::ExchangeCommand> commands{
        exchange::AddSymbolCommand{"AAPL"},
        exchange::AddSymbolCommand{"MSFT"},

        exchange::SubmitOrderCommand{
            exchange::Order{
                exchange::OrderId{1801},
                "AAPL",
                exchange::Side::Sell,
                exchange::OrderType::Limit,
                100,
                100,
                18550
            }
        },

        exchange::SubmitOrderCommand{
            exchange::Order{
                exchange::OrderId{1802},
                "AAPL",
                exchange::Side::Buy,
                exchange::OrderType::Limit,
                40,
                40,
                18550
            }
        },

        exchange::SubmitOrderCommand{
            exchange::Order{
                exchange::OrderId{1803},
                "MSFT",
                exchange::Side::Buy,
                exchange::OrderType::Limit,
                10,
                10,
                20000
            }
        },

        exchange::AmendOrderCommand{
            exchange::OrderId{1803},
            10,
            20100
        },

        exchange::CancelOrderCommand{
            exchange::OrderId{1803}
        }
    };

    exchange::ExchangeCommandLog::write(file.path(), commands);

    auto loaded =
        exchange::ExchangeCommandLog::read(file.path());

    ASSERT_EQ(loaded.size(), commands.size());

    EXPECT_TRUE(
        std::holds_alternative<exchange::AddSymbolCommand>(loaded[0])
    );
    EXPECT_TRUE(
        std::holds_alternative<exchange::SubmitOrderCommand>(loaded[2])
    );
    EXPECT_TRUE(
        std::holds_alternative<exchange::CancelOrderCommand>(loaded[6])
    );

    exchange::Exchange replayed;
    exchange::ExchangeReplay::replay(replayed, loaded);

    auto remaining =
        replayed.findActiveOrder(exchange::OrderId{1801});

    ASSERT_TRUE(remaining.has_value());
    EXPECT_EQ(remaining->remainingQuantity, 60);

    EXPECT_FALSE(
        replayed.findActiveOrder(exchange::OrderId{1802}).has_value()
    );

    EXPECT_FALSE(
        replayed.findActiveOrder(exchange::OrderId{1803}).has_value()
    );

    const auto& trades = replayed.tradesForSymbol("AAPL");

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 40);
    EXPECT_EQ(trades[0].priceInCents, 18550);
}

TEST(ExchangeCommandLogTest, RejectsUnsupportedHeader)
{
    TemporaryLog file;

    {
        std::ofstream out(file.path());
        ASSERT_TRUE(out);
        out << "MINI_EXCHANGE_COMMAND_LOG 99\n";
    }

    EXPECT_THROW(
        exchange::ExchangeCommandLog::read(file.path()),
        std::runtime_error
    );
}

TEST(ExchangeCommandLogTest, RejectsMalformedCommand)
{
    TemporaryLog file;

    {
        std::ofstream out(file.path());
        ASSERT_TRUE(out);

        out << "MINI_EXCHANGE_COMMAND_LOG 1\n";
        out << "SUBMIT_ORDER this-is-not-an-order\n";
    }

    EXPECT_THROW(
        exchange::ExchangeCommandLog::read(file.path()),
        std::runtime_error
    );
}

TEST(ExchangeCommandLogTest, RejectsMissingFile)
{
    TemporaryLog file;

    EXPECT_THROW(
        exchange::ExchangeCommandLog::read(file.path()),
        std::runtime_error
    );
}

TEST(
    ExchangeCommandLogTest,
    AppendsCommandsWithoutOverwritingExistingHistory
)
{
    TemporaryLog log;

    exchange::ExchangeCommandLog::append(
        log.path(),
        exchange::AddSymbolCommand{"AAPL"}
    );

    exchange::ExchangeCommandLog::append(
        log.path(),
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

    const auto commands =
        exchange::ExchangeCommandLog::read(log.path());

    ASSERT_EQ(commands.size(), 2U);

    EXPECT_TRUE(
        std::holds_alternative<exchange::AddSymbolCommand>(
            commands[0]
        )
    );

    EXPECT_TRUE(
        std::holds_alternative<exchange::SubmitOrderCommand>(
            commands[1]
        )
    );
}
