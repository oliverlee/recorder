#include "compat/asio.h"
#include "message.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

namespace {
using asio::ip::tcp;

/// An active connection to a sensor client streaming data
class Connection : public std::enable_shared_from_this<Connection> {
  public:
    ~Connection() { std::cerr << status_prefix_ << "Terminating connection\n"; }

  protected:
    /// @brief Creates a Connection from a socket
    /// @param socket A socket connected to a sensor client
    /// @note Objects of this class must be wrapped in the shared_ptr in order to extend the
    ///       lifetime when posting async tasks. This constructor is made 'protected' so that this
    ///       class can only be created with the free function `make_connection`.
    Connection(tcp::socket socket)
        : socket_{std::move(socket)}, status_prefix_{[&]() {
              auto ss = std::stringstream{};
              ss << "[" << socket_.remote_endpoint().address().to_string() << ":"
                 << socket_.remote_endpoint().port() << "] ";
              return ss.str();
          }()} {
        std::cerr << status_prefix_ << "Established connection\n";
    }

    /// @brief Starts reading sensor data
    auto start() -> void { async_read_header(); }

  private:
    static constexpr auto header_length = reader::wire_size::message_length;

    /// @brief Reads a message header, then posts a task to read the message payload
    auto async_read_header() -> void {
        asio::async_read(socket_,
                         streambuf_.prepare(header_length),
                         [this, self = shared_from_this()](std::error_code ec, std::size_t) {
                             if (!ec) {
                                 streambuf_.commit(header_length);
                                 const auto payload_length =
                                     reader::decode_message_payload_length(streambuf_.data());
                                 streambuf_.consume(header_length);
                                 async_read_payload(payload_length);
                             }
                         });
    }

    /// @brief Reads a message payload, prints it to stdout, then posts a task to read the next
    ///        message header
    /// @param length The message payload length
    auto async_read_payload(std::size_t length) -> void {
        asio::async_read(
            socket_,
            streambuf_.prepare(length),
            [this, self = shared_from_this()](std::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    streambuf_.commit(bytes_transferred);
                    decode_message(streambuf_.data(), std::cout, std::cerr);
                    self->streambuf_.consume(bytes_transferred);
                    self->async_read_header();
                }
            });
    }

    /// @brief Decode a message and write a JSON representation to out
    /// @param data A buffer containing an encoded message
    /// @param out An ostream to write to on success
    /// @param err An ostream to write to on failure
    /// @note Writes to err if decode fails
    /// @note Unhandled exceptions are rethrown
    auto decode_message(asio::const_buffer data, std::ostream& out, std::ostream& err) -> void {
        static constexpr decltype(nlohmann::json::type_error::id) invalid_utf8 = 316;
        static constexpr auto prefix = "[json.exception.type_error.316] ";
        static constexpr auto prefix_view = std::string_view{prefix};

        try {
            const auto message = reader::Message{data};
            out << message.as_json().dump(4) << std::endl;
        } catch (const reader::bad_message_data& ex) {
            err << status_prefix_ << "Unable to decode message: " << ex.what() << "\n";
        } catch (const nlohmann::json::type_error& ex) {
            if (ex.id == invalid_utf8) {
                err << status_prefix_ << "Unable to decode string: "
                    << std::string_view{ex.what()}.substr(prefix_view.size()) << "\n";
            } else {
                throw;
            }
        }
    }

    /// Socket to remote sensor client
    tcp::socket socket_;

    /// Internal storage for receiving encoded messages
    asio::streambuf streambuf_;

    /// Prefix for status messages containing client info
    const std::string status_prefix_;
};

/// @brief Constructs a connection from a socket
/// @param socket A socket connected to a sensor client
/// @note The spawned Connection manages its own lifetime
auto make_connection(tcp::socket socket) {
    struct helper : Connection {
        helper(tcp::socket socket) : Connection{std::move(socket)} {}

        using Connection::start;
    };

    std::make_shared<helper>(std::move(socket))->start();
}

/// A logging server
class Server {
  public:
    Server(asio::io_context& io_context, unsigned short port)
        : acceptor_{io_context, tcp::endpoint{tcp::v4(), port}} {
        do_accept();
    }

  private:
    auto do_accept() -> void {
        acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
            if (!ec) {
                make_connection(std::move(socket));
            }

            do_accept();
        });
    }

    tcp::acceptor acceptor_;
};
} // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: logger <port>\n";
        return EXIT_FAILURE;
    }

    auto io_context = asio::io_context{};

    const auto port = [&]() {
        auto ss = std::stringstream{};
        ss << argv[1];

        unsigned short p;
        ss >> p;
        return p;
    }();

    auto server = Server{io_context, port};

    std::cerr << "Starting logger on port " << port << "\n";

    io_context.run();

    return EXIT_SUCCESS;
}
