#include "exchange/ExchangeCommandLog.h"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <filesystem>
#include <utility>
#include <charconv>
#include <system_error>

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

        std::uint64_t parseUnsigned(
            const std::string& value,
            std::size_t lineNumber,
            const std::string& field
        )
        {
            std::uint64_t result{};

            const auto [end, error] = std::from_chars(
                value.data(),
                value.data() + value.size(),
                result
            );

            if (value.empty() ||
                error != std::errc{} ||
                end != value.data() + value.size())
            {
                malformed(
                    lineNumber,
                    "invalid unsigned integer for " + field
                );
            }

            return result;
        }

        std::int64_t parseSigned(
            const std::string& value,
            std::size_t lineNumber,
            const std::string& field
        )
        {
            std::int64_t result{};

            const auto [end, error] = std::from_chars(
                value.data(),
                value.data() + value.size(),
                result
            );

            if (value.empty() ||
                error != std::errc{} ||
                end != value.data() + value.size())
            {
                malformed(
                    lineNumber,
                    "invalid signed integer for " + field
                );
            }

            return result;
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
                    std::string idText;
                    std::string symbol;
                    std::string side;
                    std::string type;
                    std::string quantityText;
                    std::string remainingQuantityText;
                    std::string priceText;
                    if (!(row >> idText
                              >> std::quoted(symbol)
                              >> side
                              >> type
                              >> quantityText
                              >> remainingQuantityText
                              >> priceText))
                    {
                        malformed(lineNumber, "invalid SUBMIT_ORDER record");
                    }

                    ensureNoTrailingData(row, lineNumber);

                    return SubmitOrderCommand{
                        Order{
                        OrderId{
                            parseUnsigned(idText, lineNumber, "order ID")
                        },
                        std::move(symbol),
                        parseSide(side),
                        parseOrderType(type),
                        parseUnsigned(quantityText, lineNumber, "quantity"),
                        parseUnsigned(
                            remainingQuantityText,
                            lineNumber,
                            "remaining quantity"
                        ),
                        parseSigned(priceText, lineNumber, "price")
                    }
                };
                }

                if (kind == "CANCEL_ORDER")
                {
                    std::string idText;

                    if (!(row >> idText))
                    {
                        malformed(lineNumber, "invalid CANCEL_ORDER record");
                    }

                    ensureNoTrailingData(row, lineNumber);
                    return CancelOrderCommand{
                        OrderId{parseUnsigned(idText, lineNumber, "order ID")}
                    };
                }

                if (kind == "AMEND_ORDER")
                {
                    std::string idText;
                    std::string quantityText;
                    std::string priceText;

                    if (!(row >> idText >> quantityText >> priceText))
                    {
                        malformed(lineNumber, "invalid AMEND_ORDER record");
                    }

                    ensureNoTrailingData(row, lineNumber);

                    return AmendOrderCommand{
                        OrderId{parseUnsigned(idText, lineNumber, "order ID")},
                        parseUnsigned(quantityText, lineNumber, "new quantity"),
                        parseSigned(priceText, lineNumber, "new price")
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

    void ExchangeCommandLog::write(
        const std::string& path,
        const std::vector<ExchangeCommand>& commands
    )
    {
        // Validate and serialize all commands before touching the destination.
        std::ostringstream buffer;
        buffer << Header << '\n';

        for (const auto& command : commands)
        {
            writeCommand(buffer, command);
            buffer << '\n';

            if (!buffer)
            {
                throw std::runtime_error(
                    "Failed to serialize command log: " + path
                );
            }
        }

        const std::string contents = buffer.str();
        const std::string temporaryPath = path + ".tmp";

        {
            std::ofstream out(
                temporaryPath,
                std::ios::out | std::ios::binary | std::ios::trunc
            );

            if (!out)
            {
                throw std::runtime_error(
                    "Unable to open temporary command log: " +
                    temporaryPath
                );
            }

            out.write(
                contents.data(),
                static_cast<std::streamsize>(contents.size())
            );

            out.flush();

            if (!out)
            {
                throw std::runtime_error(
                    "Failed to write temporary command log: " +
                    temporaryPath
                );
            }

            out.close();

            if (!out)
            {
                throw std::runtime_error(
                    "Failed to close temporary command log: " +
                    temporaryPath
                );
            }
        }

        std::error_code error;
        std::filesystem::rename(temporaryPath, path, error);

        if (error)
        {
            std::error_code cleanupError;
            std::filesystem::remove(temporaryPath, cleanupError);

            throw std::runtime_error(
                "Unable to replace command log: " + path +
                " (" + error.message() + ")"
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

    void ExchangeCommandLog::append(
        const std::string& path,
        const ExchangeCommand& command
    )
    {
        // Serialize first, before modifying the destination file.
        std::ostringstream record;
        writeCommand(record, command);
        record << '\n';

        if (!record)
        {
            throw std::runtime_error(
                "Failed to serialize command for log: " + path
            );
        }

        std::error_code error;
        const bool exists = std::filesystem::exists(path, error);

        if (error)
        {
            throw std::runtime_error(
                "Unable to inspect command log: " + path
            );
        }

        bool needsHeader = !exists;

        if (exists)
        {
            const auto size = std::filesystem::file_size(path, error);

            if (error)
            {
                throw std::runtime_error(
                    "Unable to inspect command log size: " + path
                );
            }

            needsHeader = (size == 0);

            if (size > 0)
            {
                // Validate the header and every existing command before appending.
                static_cast<void>(read(path));
                std::ifstream in(path, std::ios::binary);

                if (!in)
                {
                    throw std::runtime_error(
                        "Unable to open command log for validation: " + path
                    );
                }

                std::string header;

                if (!std::getline(in, header) || header != Header)
                {
                    throw std::runtime_error(
                        "Unsupported or invalid command log header"
                    );
                }

                // Do not append to a file that ends with a partial record.
                in.clear();
                in.seekg(-1, std::ios::end);

                char lastCharacter{};

                if (!in.get(lastCharacter) || lastCharacter != '\n')
                {
                    throw std::runtime_error(
                        "Cannot append to command log with an incomplete "
                        "final record: " + path
                    );
                }
            }
        }

        // Initialize a new or empty file with the format header.
        if (needsHeader)
        {
            std::ofstream init(path, std::ios::out | std::ios::trunc);

            if (!init)
            {
                throw std::runtime_error(
                    "Unable to initialize command log: " + path
                );
            }

            init << Header << '\n';
            init.flush();

            if (!init)
            {
                throw std::runtime_error(
                    "Failed to initialize command log: " + path
                );
            }
        }

        // Append the serialized record without truncating existing history.
        std::ofstream out(path, std::ios::out | std::ios::app);

        if (!out)
        {
            throw std::runtime_error(
                "Unable to open command log for appending: " + path
            );
        }

        out << record.str();
        out.flush();

        if (!out)
        {
            throw std::runtime_error(
                "Failed to append command to log: " + path
            );
        }
    }
}