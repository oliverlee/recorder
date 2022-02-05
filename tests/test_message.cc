#include "compat/asio.h"
#include "compat/endian.h"
#include "message.h"

#include "gtest/gtest.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#ifdef __cpp_lib_filesystem
#include <filesystem>
namespace fs = std::filesystem;
#else // Assume <experimental/filesystem>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace ws = reader::wire_size;

namespace {
const auto logfile = (fs::path{__FILE__}.parent_path() / "data/testdata.log").string();

template <class T>
auto fill_impl(std::byte* p, const T& value) -> std::byte* {
    if constexpr (std::is_array_v<T>) {
        if (sizeof(T) > std::numeric_limits<std::uint8_t>::max()) {
            std::logic_error{"message name size exceeds single byte rep."};
        }
        const auto n = static_cast<uint8_t>(sizeof(T) - 1);
        std::memcpy(p, &n, 1);
        std::memcpy(p + 1, value, n);
        return p + n + 1;
    }

    std::memcpy(p, &value, sizeof(T));
    return p + sizeof(T);
}

auto fill_with_impl(std::byte* p) -> std::byte* {
    return p;
}
template <class Arg0, class... Args>
auto fill_with_impl(std::byte* p, const Arg0& a0, const Args&... args) -> std::byte* {
    return fill_with_impl(fill_impl(p, a0), args...);
}

template <class... Args>
auto fill_with(const Args&... args) {
    auto buffer = std::array<std::byte, (sizeof(Args) + ...)>{};
    const auto end = fill_with_impl(buffer.begin(), args...);

    if (static_cast<std::size_t>(end - buffer.begin()) != buffer.size()) {
        throw std::logic_error{"Buffer size does not match serialized arg size."};
    }

    return buffer;
}

} // namespace

TEST(Message, FromToolGeneratedSensorData) {
    auto sensor_data = std::ifstream{logfile, std::ios::in | std::ios_base::binary | std::ios::ate};
    ASSERT_TRUE(sensor_data.is_open());

    const auto stream_size = sensor_data.tellg();
    sensor_data.seekg(0);

    auto header_buffer = std::array<char, ws::message_length>{};
    sensor_data.read(header_buffer.data(), header_buffer.size());

    const auto payload_length = reader::decode_message_payload_length(asio::buffer(header_buffer));

    // Ensure the recorded log contains at least a full message.
    // It is assumed the beginning of the file starts with message length.
    ASSERT_GE(stream_size, payload_length + ws::message_length);

    auto payload_buffer = std::vector<char>(payload_length);
    sensor_data.read(payload_buffer.data(), payload_length);

    const auto message = reader::Message{asio::buffer(payload_buffer)};

    EXPECT_EQ("testdata", message.name());
}

class MessageWithDefaults : public ::testing::Test {
    struct timestamp_t {
        uint64_t data;
        timestamp_t(uint64_t val) : data{htobe64(val)} {}
    };

    struct temp_t {
        char buf[3];
        temp_t(uint32_t val) {
            auto const x = htobe32(val << 8);
            std::memcpy(buf, &x, 3);
        }
    };

    struct humidity_t {
        uint16_t data;
        humidity_t(uint16_t val) : data{htobe16(val)} {}
    };

  protected:
    MessageWithDefaults()
        : timestamp_ms{123}, temperature_centi_K{27315}, humidity_deci_percent{10} {}

    static constexpr uint8_t nlen = 8;
    static constexpr char name[] = "handdata";

    const timestamp_t timestamp_ms;
    const temp_t temperature_centi_K;
    const humidity_t humidity_deci_percent;

    static_assert(sizeof(name) == nlen + 1);

    const reader::Message::time_point_t expected_timestamp{std::chrono::milliseconds{123}};
};

TEST_F(MessageWithDefaults, FromHandCreatedSensorData) {
    const auto data = fill_with(timestamp_ms, name, temperature_centi_K, humidity_deci_percent);
    const auto message = reader::Message{asio::buffer(data)};

    EXPECT_EQ(expected_timestamp, message.timestamp());
    EXPECT_EQ("handdata", message.name());
    EXPECT_EQ(0.0f, message.temperature());
    EXPECT_EQ(1.0f, message.humidity());
}

TEST_F(MessageWithDefaults, HasTemperatureButNoHumidity) {
    const auto data = fill_with(timestamp_ms, name, temperature_centi_K);
    const auto message = reader::Message{asio::buffer(data)};

    EXPECT_EQ(expected_timestamp, message.timestamp());
    EXPECT_EQ("handdata", message.name());
    EXPECT_EQ(0.0f, message.temperature());
    EXPECT_EQ(std::nullopt, message.humidity());
}

TEST_F(MessageWithDefaults, HasHumidityButNoTemperature) {
    const auto data = fill_with(timestamp_ms, name, humidity_deci_percent);
    const auto message = reader::Message{asio::buffer(data)};

    EXPECT_EQ(expected_timestamp, message.timestamp());
    EXPECT_EQ("handdata", message.name());
    EXPECT_EQ(std::nullopt, message.temperature());
    EXPECT_EQ(1.0f, message.humidity());
}

TEST_F(MessageWithDefaults, BufferTooLarge) {
    const auto data = fill_with(timestamp_ms, name, temperature_centi_K, humidity_deci_percent);
    auto data2 = std::array<std::byte, 1 + std::tuple_size_v<decltype(data)>>{};
    std::copy(data.cbegin(), data.cend(), data2.begin());

    EXPECT_THROW(reader::Message{asio::buffer(data2)};, reader::bad_message_data);
}

TEST(Message, BufferTooSmall) {
    std::array<std::byte, ws::timestamp> data;

    EXPECT_THROW(reader::Message{asio::buffer(data)};, reader::bad_message_data);
}

TEST(Message, NLenTooLargeForBuffer) {
    const uint64_t timestamp_ms = htobe64(123);
    constexpr uint8_t incorrect_nlen = 200;
    constexpr uint8_t correct_nlen = 8;
    const char* name = "handdata";

    std::array<std::byte, ws::timestamp + ws::nlen + correct_nlen> data;
    std::memcpy(&data[0], &timestamp_ms, ws::timestamp);
    std::memcpy(&data[ws::timestamp], &incorrect_nlen, ws::nlen);
    std::memcpy(&data[ws::timestamp + ws::nlen], name, correct_nlen);

    EXPECT_THROW(reader::Message{asio::buffer(data)};, reader::bad_message_data);
}
