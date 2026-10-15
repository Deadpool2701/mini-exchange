#include "exchange/ExchangeCommandLog.h"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <variant>

namespace exchange
{
    namespace
    {
        constexpr const char* Header =
            "MINI_EXCHANGE_COMMAND_LOG 1";

        [[noreturn]] void malformed( std::size_t lineNumber, const std::string& reason )
        {
            throw std::runtime_error(
                "Invalid command log at line " +
                std::to_string(lineNumber) + ": " + reason
            );
        }

        void ensureNoTrailingData( std::istringstream& row, std::size_t lineNumber )
        {
            std::string extra;

            if (row >> extra)
            {
                malformed(lineNumber, "unexpected trailing data");
            }
        }

        void validateLogSymbol(const std::string& symbol)
        {
            // std::quoted escapes quotes and backslashes, but does
            // not encode newlines as a single-line record.
            if (symbol.find('\n') != std::string::npos ||
                symbol.find('\r') != std::string::npos)
            {
                throw std::invalid_argument(
                    "Symbols containing newline characters cannot "
                    "be written to the command log"
                );
            }
        }

        const char* sideName(Side side)
        {
            switch (side)
            {
                case Side::Buy:
                    return "BUY";

                case Side::Sell:
                    return "SELL";
            }

            throw std::invalid_argument("Invalid order side");
        }

        const char* orderTypeName(OrderType type)
        {
            switch (type)
            {
                case OrderType::Limit:
                    return "LIMIT";

                case OrderType::Market:
                    return "MARKET";
            }

            throw std::invalid_argument("Invalid order type");
        }

        Side parseSide(const std::string& value)
        {
            if (value == "BUY")
            {
                return Side::Buy;
            }

            if (value == "SELL")
            {
                return Side::Sell;
            }

            throw std::invalid_argument("Unknown order side");
        }

        OrderType parseOrderType(const std::string& value)
        {
            if (value == "LIMIT")
            {
                return OrderType::Limit;
            }

            if (value == "MARKET")
            {
                return OrderType::Market;
            }

            throw std::invalid_argument("Unknown order type");
        }

        void writeCommand( std::ostream& out, const ExchangeCommand& command )
        {
            std::visit(
                [&out](const auto& cmd)
                {
                    using T = std::decay_t<decltype(cmd)>;

                    if constexpr (
                        std::is_same_v<T, AddSymbolCommand>
                    )
                    {
                        validateLogSymbol(cmd.symbol);
                        out << "ADD_SYMBOL "
                            << std::quoted(cmd.symbol);
                    }
                    else if constexpr (
                        std::is_same_v<T, SubmitOrderCommand>
                    )
                    {
                        const Order& order = cmd.order;
                        validateLogSymbol(order.symbol);

                        out << "SUBMIT_ORDER "
                            << order.id.value << ' '
                            << std::quoted(order.symbol) << ' '
                            << sideName(order.side) << ' '
                            << orderTypeName(order.type) << ' '
                            << order.quantity << ' '
                            << order.remainingQuantity << ' '
                            << order.priceInCents;
                    }
                    else if constexpr (
                        std::is_same_v<T, CancelOrderCommand>
                    )
                    {
                        out << "CANCEL_ORDER "
                            << cmd.orderId.value;
                    }
                    else if constexpr (
                        std::is_same_v<T, AmendOrderCommand>
                    )
                    {
                        out << "AMEND_ORDER "
                            << cmd.orderId.value << ' '
                            << cmd.newQuantity << ' '
                            << cmd.newPriceInCents;
                    }
                },
                command
            );
        }

        ExchangeCommand parseCommand( const std::string& line, std::size_t lineNumber)
        {
            std::istringstream row(line);
            std::string kind;

            if (!(row >> kind))
            {
                malformed(lineNumber, "missing command type");
            }

            try
            {
                if (kind == "ADD_SYMBOL")
                {
                    std::string symbol;

                    if (!(row >> std::quoted(symbol)))
                    {
                        malformed(lineNumber, "invalid ADD_SYMBOL record");
                    }

                    ensureNoTrailingData(row, lineNumber);
                    return AddSymbolCommand{std::move(symbol)};
                }

                if (kind == "SUBMIT_ORDER")
                {
                    std::uint64_t id;
                    std::string symbol;
                    std::string side;
                    std::string type;
                    std::uint64_t quantity;
                    std::uint64_t remainingQuantity;
                    std::int64_t priceInCents;

                    if (!(row >> id
                              >> std::quoted(symbol)
                              >> side
                              >> type
                              >> quantity
                              >> remainingQuantity
                              >> priceInCents))
                    {
                        malformed(lineNumber, "invalid SUBMIT_ORDER record");
                    }

                    ensureNoTrailingData(row, lineNumber);

                    return SubmitOrderCommand{
                        Order{
                            OrderId{id},
                            std::move(symbol),
                            parseSide(side),
                            parseOrderType(type),
                            quantity,
                            remainingQuantity,
                            priceInCents
                        }
                    };
                }

                if (kind == "CANCEL_ORDER")
                {
                    std::uint64_t id;

                    if (!(row >> id))
                    {
                        malformed(lineNumber, "invalid CANCEL_ORDER record");
                    }

                    ensureNoTrailingData(row, lineNumber);
                    return CancelOrderCommand{OrderId{id}};
                }

                if (kind == "AMEND_ORDER")
                {
                    std::uint64_t id;
                    std::uint64_t newQuantity;
                    std::int64_t newPriceInCents;

                    if (!(row >> id >> newQuantity >> newPriceInCents))
                    {
                        malformed(lineNumber, "invalid AMEND_ORDER record");
                    }

                    ensureNoTrailingData(row, lineNumber);

                    return AmendOrderCommand{
                        OrderId{id},
                        newQuantity,
                        newPriceInCents
                    };
                }

                malformed(lineNumber, "unknown command type: " + kind);
            }
            catch (const std::invalid_argument& error)
            {
                malformed(lineNumber, error.what());
            }
        }
    }

    void ExchangeCommandLog::write( const std::string& path, const std::vector<ExchangeCommand>& commands )
    {
        std::ofstream out(path, std::ios::out | std::ios::trunc);

        if (!out)
        {
            throw std::runtime_error(
                "Unable to open command log for writing: " + path
            );
        }

        out << Header << '\n';

        for (const auto& command : commands)
        {
            writeCommand(out, command);
            out << '\n';

            if (!out)
            {
                throw std::runtime_error(
                    "Failed while writing command log: " + path
                );
            }
        }

        out.flush();

        if (!out)
        {
            throw std::runtime_error(
                "Failed to flush command log: " + path
            );
        }
    }

    std::vector<ExchangeCommand> ExchangeCommandLog::read( const std::string& path )
    {
        std::ifstream in(path);

        if (!in)
        {
            throw std::runtime_error(
                "Unable to open command log for reading: " + path
            );
        }

        std::string line;

        if (!std::getline(in, line))
        {
            throw std::runtime_error("Command log is empty");
        }

        if (line != Header)
        {
            throw std::runtime_error(
                "Unsupported or invalid command log header"
            );
        }

        std::vector<ExchangeCommand> commands;
        std::size_t lineNumber = 1;

        while (std::getline(in, line))
        {
            ++lineNumber;

            if (line.empty())
            {
                continue;
            }

            commands.push_back(parseCommand(line, lineNumber));
        }

        if (in.bad())
        {
            throw std::runtime_error(
                "I/O error while reading command log: " + path
            );
        }

        return commands;
    }
}