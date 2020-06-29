#include "compat/asio.h"
#include "connection.h"

#include <iostream>

namespace {
using asio::ip::tcp;

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
                logger::make_connection(std::move(socket), std::cout, std::cerr);
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
