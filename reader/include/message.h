#pragma once

#include "aux.h"
#include "compat/asio.h"

#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>

namespace reader {

/// Thrown on decode error
struct bad_message_data : public std::length_error {
    bad_message_data(const char* what_arg) : length_error(what_arg) {}
};

/// Represents a sensor data message
class Message {
  public:
    using clock_t = std::chrono::system_clock;
    using time_point_t = std::chrono::time_point<clock_t>;

    /// @brief Constructs a message by decoding bytes in wire format
    /// @param wire_data A const buffer to decode which contains the message payload
    /// @throw bad_message_data if `wire_data` is too small to contain a message
    /// @throw bad_message_data if the decoded name has a size inconsistent with the message
    /// @throw bad_message_data if `wire_size` contains unused bytes after decoding
    /// @note The payload is not expected to contain the message length header
    Message(asio::const_buffer wire_data);

    auto timestamp() const noexcept -> time_point_t { return timestamp_; }
    auto name() const noexcept -> const std::string& { return name_; }
    auto temperature() const noexcept -> std::optional<float> { return temperature_; }
    auto humidity() const noexcept -> std::optional<float> { return humidity_; }

  private:
    auto set_timestamp(asio::const_buffer wire_data) -> void;
    auto set_name(asio::const_buffer wire_data) -> void;
    auto set_temperature(asio::const_buffer wire_data) -> void;
    auto set_humidity(asio::const_buffer wire_data) -> void;

    time_point_t timestamp_;
    std::string name_;
    std::optional<float> temperature_;
    std::optional<float> humidity_;
};

/// @brief Writes a `Message` to an output stream
auto operator<<(std::ostream& out, const Message& message) -> std::ostream&;

} // namespace reader
