#include "message.h"

#include <cstdint>
#include <iomanip>

#ifdef __APPLE__
#include "compat/endian.h"
#else
#include <endian.h>
#endif

namespace {

namespace wire_size {
constexpr std::size_t timestamp = 8;
constexpr std::size_t nlen = 1;
constexpr std::size_t temperature = 3;
constexpr std::size_t humidity = 2;
} // namespace wire_size

} // namespace

namespace reader {

Message::Message(asio::const_buffer data) {
    constexpr auto minimum_message_size = wire_size::timestamp + wire_size::nlen;
    if (data.size() < minimum_message_size) {
        throw bad_message_data{"Message data buffer is smaller than expected."};
    };

    const auto timestamp_buffer = asio::buffer(data, wire_size::timestamp);

    std::size_t offset = timestamp_buffer.size();
    const auto nlen = aux::buffer_cast<uint8_t>(asio::buffer(data + offset, wire_size::nlen));

    offset += wire_size::nlen;
    const auto name_buffer = asio::buffer(data + offset, nlen);

    offset += name_buffer.size();
    const auto temperature_buffer = asio::buffer(data + offset, wire_size::temperature);

    offset += temperature_buffer.size();
    const auto humidity_buffer = asio::buffer(data + offset);

    timestamp_ = time_point_t{
        std::chrono::milliseconds{be64toh(aux::buffer_cast<uint64_t>(timestamp_buffer))}};

    name_ = std::string{static_cast<const char*>(name_buffer.data()), name_buffer.size()};

    if (temperature_buffer.size() == wire_size::temperature) {
        uint32_t temp_be;
        std::memcpy(&temp_be, temperature_buffer.data(), temperature_buffer.size());

        auto temp_he = be32toh(temp_be);

        // Temperature is only 3 bytes, not 4. So we need to shift if the machine is little endian.
        // If the machine is big endian, then be32toh() will do nothing.
        if (temp_he != temp_be) {
            temp_he >>= 8;
        }

        temperature_ = static_cast<float>(temp_he);
    }

    if (humidity_buffer.size() == wire_size::humidity) {
        humidity_ = static_cast<float>(be16toh(aux::buffer_cast<uint16_t>(humidity_buffer)));
    }
}

std::ostream& operator<<(std::ostream& out, const Message::time_point_t& timestamp) {
    constexpr auto iso_8601_date_time = "%FT%T";
    constexpr auto iso_8601_timezone = "%z";

    const auto time = Message::clock_t::to_time_t(timestamp);
    const auto localtime = std::localtime(&time);

    const auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;

    out << std::put_time(localtime, iso_8601_date_time);
    out << '.' << std::setfill('0') << std::setw(3) << ms.count();
    out << std::put_time(localtime, iso_8601_timezone);

    return out;
}

std::ostream& operator<<(std::ostream& out, const Message& message) {
    out << "Message {\n";
    out << "  timestamp: " << message.timestamp() << "\n";
    out << "  name: " << message.name() << "\n";
    out << "  temperature: " << message.temperature() << "\n";
    out << "  humidity: " << message.humidity() << "\n";
    out << "}";

    return out;
}

} // namespace reader
