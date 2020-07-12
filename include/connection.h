#pragma once

#include "compat/asio.h"
#include "message.h"
#include "nonstd/expected.hpp"

#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

namespace logger {

/// An active connection to a sensor client streaming data
/// @tparam AsyncReadSocketStream A type that extends asio::AsyncReadStream
///
/// A Connection<AsyncReadSocketStream> is a type that reads sensor data and decodes messages.
/// The AsyncReadSocketStream type is expected to satisfy the requirements for asio::AsyncReadStream
/// and provide the additional functionality:
/// - AsyncReadSocketStream::remote_endpoint().address().to_string()
/// - AsyncReadSocketStream::remote_endpoint().port()
///
/// @see https://think-async.com/Asio/asio-1.16.1/doc/asio/reference/AsyncReadStream.html
template <class AsyncReadSocketStream>
class Connection : public std::enable_shared_from_this<Connection<AsyncReadSocketStream>> {
  public:
    ~Connection() { err_ << status_prefix_ << "Terminating connection\n"; }

  protected:
    /// @brief Creates a Connection from a socket
    /// @param socket A socket connected to a sensor client
    /// @param out An ostream to write to on success
    /// @param err An ostream to write to on failure
    /// @note The Connection manages its own lifetime
    /// @note `out` and `err` must be guaranteed to exist for the lifetime of the Connection
    /// @note Objects of this class must be wrapped in the shared_ptr in order to extend the
    ///       lifetime when posting async tasks. This constructor is made 'protected' so that this
    ///       class can only be created with the free function `make_connection`.
    Connection(AsyncReadSocketStream socket, std::ostream& out, std::ostream& err)
        : socket_{std::move(socket)},
          status_prefix_{[&]() {
              auto ss = std::stringstream{};
              ss << "[" << socket_.remote_endpoint().address().to_string() << ":"
                 << socket_.remote_endpoint().port() << "] ";
              return ss.str();
          }()},
          out_{out},
          err_{err} {
        err_ << status_prefix_ << "Established connection\n";
    }

    /// @brief Starts reading sensor data
    auto start() -> void { display_message(); }

  private:
    static constexpr auto header_length = reader::wire_size::message_length;
    using expected_type = nonstd::expected<reader::Message, std::exception_ptr>;

    /// @brief Reads a message header and writes it to `out_`
    auto display_message() -> void {
        async_receive_message([this, self = this->shared_from_this()](
                                  const std::error_code& ec, const expected_type& message) {
            if (ec) {
                err_ << status_prefix_ << ec << std::endl;
                return;
            }

            if (message) {
                try {
                    out_ << message->as_json().dump(4) << std::endl;
                } catch (const nlohmann::json::type_error& ex) {
                    static constexpr int invalid_utf8 = 316;
                    static constexpr auto prefix = "[json.exception.type_error.316] ";
                    static constexpr auto prefix_view = std::string_view{prefix};

                    if (ex.id != invalid_utf8) {
                        throw;
                    }
                    err_ << status_prefix_ << "Unable to decode string: "
                         << std::string_view{ex.what()}.substr(prefix_view.size()) << "\n";
                }
            } else {
                try {
                    std::rethrow_exception(message.error());
                } catch (const reader::bad_message_data& ex) {
                    err_ << status_prefix_ << "Unable to decode message: " << ex.what() << "\n";
                }
            }

            display_message();
        });
    }

#include <asio/yield.hpp>

    /// @brief Reads a message
    /// @param token A token that determines the handler invoked when the asynchronous operation is
    /// completed
    ///
    /// Reads a message header followed by a message payload.
    template <class CompletionToken>
    auto async_receive_message(CompletionToken&& token) {
        return asio::async_compose<CompletionToken,
                                   void(const std::error_code&, const expected_type&)>(
            [conn = this, coro = asio::coroutine()](auto& self,
                                                    const std::error_code& error = {},
                                                    std::size_t bytes_transferred = 0) mutable {
                reenter(coro) {
                    assert(conn->streambuf_.size() == 0);
                    yield asio::async_read(
                        conn->socket_, conn->streambuf_.prepare(header_length), std::move(self));
                    if (error) {
                        return self.complete(error,
                                             nonstd::make_unexpected<std::exception_ptr>({}));
                    }
                    conn->streambuf_.commit(header_length);

                    yield {
                        const auto payload_length =
                            reader::decode_message_payload_length(conn->streambuf_.data());
                        conn->streambuf_.consume(header_length);
                        asio::async_read(conn->socket_,
                                         conn->streambuf_.prepare(payload_length),
                                         std::move(self));
                    }
                    if (error) {
                        return self.complete(error,
                                             nonstd::make_unexpected<std::exception_ptr>({}));
                    }
                    conn->streambuf_.commit(bytes_transferred);

                    auto message = [](auto buffer) noexcept->expected_type {
                        try {
                            return reader::Message{buffer};
                        } catch (...) {
                            return nonstd::make_unexpected(std::current_exception());
                        }
                    }
                    (conn->streambuf_.data());
                    conn->streambuf_.consume(conn->streambuf_.size());
                    self.complete(error, std::move(message));
                }
            },
            token);
    }

#include <asio/unyield.hpp>

    /// Socket to remote sensor client
    AsyncReadSocketStream socket_;

    /// Internal storage for receiving encoded messages
    asio::streambuf streambuf_;

    /// Prefix for status messages containing client info
    const std::string status_prefix_;

    /// Output stream to write sensor messages to
    std::ostream& out_;

    /// Output stream to write status/error messages to
    std::ostream& err_;
};

/// @brief Constructs a connection from a socket
/// @tparam AsyncReadSocketStream A type that extends asio::AsyncReadStream
/// @param socket A socket connected to a sensor client
/// @param out An ostream to write to on success
/// @param err An ostream to write to on failure
/// @note The spawned Connection manages its own lifetime
/// @note `out` and `err` must be guaranteed to exist for the lifetime of the Connection
/// @see Connection
/// @see https://think-async.com/Asio/asio-1.16.1/doc/asio/reference/AsyncReadStream.html
template <class AsyncReadSocketStream>
auto make_connection(AsyncReadSocketStream socket, std::ostream& out, std::ostream& err) {
    struct helper : Connection<AsyncReadSocketStream> {
        helper(AsyncReadSocketStream socket, std::ostream& out, std::ostream& err)
            : Connection<AsyncReadSocketStream>{std::move(socket), out, err} {}

        using Connection<AsyncReadSocketStream>::start;
    };

    std::make_shared<helper>(std::move(socket), out, err)->start();
}

} // namespace logger
