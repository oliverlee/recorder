#include "compat/asio.h"
#include "message.h"

#include <iostream>
#include <memory>
#include <sstream>

namespace {
using asio::ip::tcp;

/// An active connection to a sensor client streaming data
class Session : public std::enable_shared_from_this<Session> {
  public:
    ~Session() { std::cerr << "Terminating connection\n"; }

  protected:
    /// @brief Creates a Session from a socket
    /// @param socket A socket connected to a sensor client
    /// @note Objects of this class must be wrapped in the shared_ptr in order to extend the
    ///       lifetime when posting async tasks. This constructor is made 'protected' so that this
    ///       class can only be created with the free function `make_session`.
    Session(tcp::socket socket) : socket_{std::move(socket)} {}

    /// @brief Starts reading sensor data
    auto start() -> void { async_read_header(); }

  private:
    static constexpr auto header_length = reader::wire_size::message_length;

    ///@brief Reads a message header, then posts a task to read the message payload
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

    ///@brief Reads a message payload, prints it to stdout, then posts a task to read the next
    ///       message header
    ///@param length The message payload length
    auto async_read_payload(std::size_t length) -> void {
        asio::async_read(
            socket_,
            streambuf_.prepare(length),
            [this, self = shared_from_this()](std::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    streambuf_.commit(bytes_transferred);
                    const auto message = reader::Message{streambuf_.data()};
                    self->streambuf_.consume(bytes_transferred);

                    std::cout << message.as_json().dump(4) << "\n";

                    self->async_read_header();
                }
            });
    }

    /// Socket to remote sensor client
    tcp::socket socket_;

    /// Internal storage for receiving encoded messages
    asio::streambuf streambuf_;
};

/// @brief Constructs a session from a socket
/// @param socket A socket connected to a sensor client
/// @note The spawned Session manages its own lifetime
auto make_session(tcp::socket socket) {
    struct helper : Session {
        helper(tcp::socket socket) : Session{std::move(socket)} {}

        using Session::start;
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
                std::cerr << "Established connection\n";
                make_session(std::move(socket));
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

    asio::io_context io_context;

    const auto port = [](auto s) {
        auto ss = std::stringstream{};
        ss << s;

        unsigned short p;
        ss >> p;
        return p;
    }(argv[1]);

    Server s{io_context, port};

    std::cerr << "Starting logger on port " << port << "\n";

    io_context.run();

    return EXIT_SUCCESS;
}
