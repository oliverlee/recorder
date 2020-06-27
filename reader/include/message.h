#pragma once

#ifdef __APPLE__
#if defined(__GNUC__) && !defined(__clang__)
// Workaround for XCode
// https://github.com/Homebrew/homebrew-core/issues/40676
#include <cstdio>
#include <cstdlib>
#endif
#endif

#include "asio.hpp"
#include "aux.h"

#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>

namespace reader {

struct bad_message_data : public std::runtime_error {
    bad_message_data(const char* what_arg) : runtime_error(what_arg) {}
};

// A sensor data message
class Message {
  public:
    using clock_t = std::chrono::system_clock;
    using time_point_t = std::chrono::time_point<clock_t>;

    // Constructs a message by decoding bytes in wire format
    Message(asio::const_buffer data);

    auto timestamp() const noexcept -> time_point_t { return timestamp_; }
    auto name() const noexcept -> const std::string& { return name_; }
    auto temperature() const noexcept -> std::optional<float> { return temperature_; }
    auto humidity() const noexcept -> std::optional<float> { return humidity_; }

  private:
    time_point_t timestamp_;
    std::string name_;
    std::optional<float> temperature_;
    std::optional<float> humidity_;
};

template <class T>
std::ostream& operator<<(std::ostream& out, const std::optional<T>& t) {
    out << "[";
    if (t) {
        out << *t;
    }
    out << "]";

    return out;
}

std::ostream& operator<<(std::ostream& out, const Message::time_point_t& timestamp);

std::ostream& operator<<(std::ostream& out, const Message& message);

} // namespace reader
