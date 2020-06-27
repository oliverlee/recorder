#include "compat/asio.h"

#include <iostream>
#include <sstream>

namespace {
using asio::ip::tcp;

class Server {
  public:
    Server(asio::io_context& io_context, unsigned short port)
        : acceptor_{io_context, tcp::endpoint{tcp::v4(), port}} {
        do_accept();
    }

  private:
    void do_accept() {
        acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
            if (!ec) {
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
