#include "message.h"

#include "compat/endian.h"

#include <cstdint>
#include <iomanip>

namespace {

namespace wire_size {
constexpr std::size_t timestamp = 8;
constexpr std::size_t nlen = 1;
constexpr std::size_t temperature = 3;
constexpr std::size_t humidity = 2;
} // namespace wire_size

} // namespace

namespace reader {

template <class T>
auto operator<<(std::ostream& out, const std::optional<T>& t) -> std::ostream& {
    out << "[";
    if (t) {
        out << *t;
    }
    out << "]";

    return out;
}

auto operator<<(std::ostream& out, const reader::Message::time_point_t& timestamp)
    -> std::ostream& {
    constexpr auto iso_8601_date_time = "%FT%T";
    constexpr auto iso_8601_timezone = "%z";

    const auto time = reader::Message::clock_t::to_time_t(timestamp);
    const auto localtime = std::localtime(&time);

    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;

    out << std::put_time(localtime, iso_8601_date_time);
    out << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
    out << std::put_time(localtime, iso_8601_timezone);

    return out;
}

Message::Message(asio::const_buffer wire_data) {
    constexpr auto minimum_message_size = wire_size::timestamp + wire_size::nlen;
    if (wire_data.size() < minimum_message_size) {
        throw bad_message_data{"Message wire data buffer is smaller than expected."};
    };

    const auto timestamp_buffer = asio::buffer(wire_data, wire_size::timestamp);

    std::size_t offset = timestamp_buffer.size();
    const auto nlen = aux::buffer_cast<uint8_t>(asio::buffer(wire_data + offset, wire_size::nlen));

    offset += wire_size::nlen;
    const auto name_buffer = asio::buffer(wire_data + offset, nlen);

    offset += name_buffer.size();
    const auto temperature_buffer = asio::buffer(wire_data + offset, wire_size::temperature);

    offset += temperature_buffer.size();
    const auto humidity_buffer = asio::buffer(wire_data + offset);

    set_timestamp(timestamp_buffer);

    if (name_buffer.size() != nlen) {
        throw bad_message_data{"`nlen` exceeds wire data buffer length."};
    }
    set_name(name_buffer);

    if (temperature_buffer.size() == wire_size::temperature) {
        set_temperature(temperature_buffer);
    }

    if (humidity_buffer.size() == wire_size::humidity) {
        set_humidity(humidity_buffer);
    }
}

auto Message::set_timestamp(asio::const_buffer wire_data) -> void {
    timestamp_ =
        time_point_t{std::chrono::milliseconds{be64toh(aux::buffer_cast<uint64_t>(wire_data))}};
}

auto Message::set_name(asio::const_buffer wire_data) -> void {
    name_ = std::string{static_cast<const char*>(wire_data.data()), wire_data.size()};
}

auto Message::set_temperature(asio::const_buffer wire_data) -> void {
    // converts hundredths of K to C
    const auto convert_unit = [](float f) { return f / 100.0f - 273.15f; };

    uint32_t temp_be;
    std::memcpy(&temp_be, wire_data.data(), wire_data.size());

    auto temp_he = be32toh(temp_be);

    // Temperature is only 3 bytes, not 4. So we need to shift if the machine is little endian.
    // If the machine is big endian, then ntohl() will do nothing.
    if (temp_he != temp_be) {
        temp_he >>= 8;
    }

    temperature_ = convert_unit(static_cast<float>(temp_he));
}

auto Message::set_humidity(asio::const_buffer wire_data) -> void {
    // converts relative humidity in ‰ to %
    const auto convert_unit = [](float f) { return f / 10.0f; };

    humidity_ = convert_unit(static_cast<float>(be16toh(aux::buffer_cast<uint16_t>(wire_data))));
}

auto operator<<(std::ostream& out, const Message& message) -> std::ostream& {
    out << "Message {\n";
    out << "  timestamp: " << message.timestamp() << "\n";
    out << "  name: " << message.name() << "\n";
    out << "  temperature: " << message.temperature() << "\n";
    out << "  humidity: " << message.humidity() << "\n";
    out << "}";

    return out;
}

} // namespace reader
