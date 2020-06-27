#include "compat/asio.h"
#include "message.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

class Reader {
  public:
    /// @brief Decode bytes from an istream
    /// @param in An istream to read from
    auto decode_istream(std::istream& in) -> void;

  private:
    /// @brief Decodes a message payload length from an istream
    /// @param in An istream to read from
    /// @return Payload length of the following message
    auto decode_header(std::istream& in) -> std::size_t;

    /// @brief Decodes a message payload length from an istream
    /// @param in An istream to read from
    /// @return A Message decoded from the istream payload
    auto decode_payload(std::istream& in, std::size_t length) -> reader::Message;

    std::array<std::byte, reader::wire_size::message_length> header_buffer_;
    std::vector<std::byte> payload_buffer_;
};

/// @brief Reads bytes from an istream into a buffer until the buffer is full
auto read_until_full(std::istream& in, asio::mutable_buffer buffer) -> void {
    while (buffer.size() > 0) {
        in.read(static_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        buffer += static_cast<std::size_t>(in.gcount());
    }
}

auto Reader::decode_istream(std::istream& in) -> void {
    while (true) {
        const auto payload_length = decode_header(in);
        const auto message = decode_payload(in, payload_length);
        std::cout << message.as_json().dump(4) << "\n";
    }
}

auto Reader::decode_header(std::istream& in) -> std::size_t {
    auto buffer = asio::buffer(header_buffer_);
    read_until_full(in, buffer);
    return reader::decode_message_payload_length(buffer);
}

auto Reader::decode_payload(std::istream& in, std::size_t length) -> reader::Message {
    payload_buffer_.resize(length);

    auto buffer = asio::buffer(payload_buffer_);
    read_until_full(in, buffer);
    return reader::Message{buffer};
}

} // namespace

int main() {
    Reader().decode_istream(std::cin);

    return EXIT_SUCCESS;
}
