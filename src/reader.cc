#include "compat/asio.h"

#include <cstdint>
#include <iostream>
#include <system_error>

namespace {
using asio::ip::tcp;
using asio::posix::stream_descriptor;

/// Sends sensor data stream to a log server
class Client {
  public:
    Client(tcp::socket socket, stream_descriptor istream)
        : socket_{std::move(socket)}, istream_{std::move(istream)} {
        async_read_data();
    }

  private:
    /// @brief Writes the internal buffer to the log server, then posts a read
    auto async_write_data() -> void {
        asio::async_write(socket_, streambuf_, [this](std::error_code ec, std::size_t) {
            if (!ec) {
                async_read_data();
            }
        });
    }

    /// @brief Reads from the input stream until the internal buffer is full, then posts a write
    auto async_read_data() -> void {
        asio::async_read(istream_, streambuf_, [this](std::error_code ec, std::size_t) {
            if (!ec) {
                async_write_data();
            }
        });
    }

    /// Maximum size of the streambuf
    static constexpr std::size_t bufsize = 16;

    /// Socket to remote log server
    tcp::socket socket_;

    /// Input data stream
    stream_descriptor istream_;

    /// Internal buffer for forwarding from the input data stream to the log server
    asio::streambuf streambuf_{bufsize};
};
} // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "usage: reader <host> <port>\n";
        return EXIT_FAILURE;
    }

    asio::io_context io_context;

    asio::posix::stream_descriptor istream{io_context, STDIN_FILENO};

    asio::ip::tcp::socket socket{io_context};
    try {
        asio::connect(socket, asio::ip::tcp::resolver{io_context}.resolve(argv[1], argv[2]));
    } catch (const std::system_error& ex) {
        std::cerr << "Unable to connect to " << argv[1] << ":" << argv[2] << "\n";
        std::cerr << ex.what() << "\n";
        std::cerr << "Is a log server running?\n";

        return EXIT_FAILURE;
    }

    auto client = Client{std::move(socket), std::move(istream)};

    io_context.run();

    return EXIT_SUCCESS;
}
