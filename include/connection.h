#pragma once

#include "compat/asio.h"
#include "message.h"
#include "nonstd/expected.hpp"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

namespace logger {

#include <asio/yield.hpp>

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
    // auto start() -> void { async_read_header(); }
    auto start() -> void { async_display_message(); }

  private:
    static constexpr auto header_length = reader::wire_size::message_length;

    auto async_display_message() -> void {
        async_receive_message(
            [this, self = this->shared_from_this()](const std::error_code& ec,
                                                    const std::optional<reader::Message>& message) {
                if (!ec && message) {
                    out_ << message->as_json().dump(4) << std::endl;
                    async_display_message();
                }
            });
    }

    template <class CompletionToken>
    auto async_receive_message(CompletionToken&& token) {
        // handler signature must have parameters that are default constructible
        // TODO: replace optional with expected/outcome so we can handle exceptions thrown on
        // Message construction
        return asio::async_compose<CompletionToken,
                                   void(const std::error_code&,
                                        const std::optional<reader::Message>&)>(
            [conn = this, coro = asio::coroutine()](auto& self, // `self` refers to the handler
                                                    const std::error_code& error = {},
                                                    std::size_t bytes_transferred = 0) mutable {
                reenter(coro) {
                    assert(conn->streambuf_.size() == 0);
                    yield asio::async_read(
                        conn->socket_, conn->streambuf_.prepare(header_length), std::move(self));
                    if (error) {
                        return self.complete(error, {});
                    }

                    yield {
                        conn->streambuf_.commit(header_length);
                        const auto payload_length =
                            reader::decode_message_payload_length(conn->streambuf_.data());
                        conn->streambuf_.consume(header_length);
                        asio::async_read(conn->socket_,
                                         conn->streambuf_.prepare(payload_length),
                                         std::move(self));
                    }
                    conn->streambuf_.commit(bytes_transferred);

                    if (error) {
                        return self.complete(error, {});
                    }

                    const auto message =
                        std::optional<reader::Message>{std::in_place, conn->streambuf_.data()};
                    conn->streambuf_.consume(bytes_transferred);
                    self.complete(error, message);
                }
            },
            token);
    }

    /// @brief Reads a message header, then posts a task to read the message payload
    auto async_read_header() -> void {
        asio::async_read(socket_,
                         streambuf_.prepare(header_length),
                         [this, self = this->shared_from_this()](std::error_code ec, std::size_t) {
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
        asio::async_read(socket_,
                         streambuf_.prepare(length),
                         [this, self = this->shared_from_this()](std::error_code ec,
                                                                 std::size_t bytes_transferred) {
                             if (!ec) {
                                 streambuf_.commit(bytes_transferred);
                                 decode_message(streambuf_.data());
                                 streambuf_.consume(bytes_transferred);
                                 async_read_header();
                             }
                         });
    }

    /// @brief Decode a message and write a JSON representation to out
    /// @param data A buffer containing an encoded message
    /// @note Writes to err if decode fails
    /// @note Unhandled exceptions are rethrown
    auto decode_message(asio::const_buffer data) -> void {
        static constexpr int invalid_utf8 = 316;
        static constexpr auto prefix = "[json.exception.type_error.316] ";
        static constexpr auto prefix_view = std::string_view{prefix};

        try {
            const auto message = reader::Message{data};
            out_ << message.as_json().dump(4) << std::endl;
        } catch (const reader::bad_message_data& ex) {
            err_ << status_prefix_ << "Unable to decode message: " << ex.what() << "\n";
        } catch (const nlohmann::json::type_error& ex) {
            if (ex.id == invalid_utf8) {
                err_ << status_prefix_ << "Unable to decode string: "
                     << std::string_view{ex.what()}.substr(prefix_view.size()) << "\n";
            } else {
                throw;
            }
        }
    }

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

#include <asio/unyield.hpp>

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
