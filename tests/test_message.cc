#include "compat/asio.h"
#include "compat/endian.h"
#include "message.h"

#include "gtest/gtest.h"
#include <array>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <string>
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
  protected:
    MessageWithDefaults()
        : timestamp_ms{htobe64(123)},
          temperature_centi_K{htobe32(static_cast<uint32_t>(27315) << 8)},
          humidity_deci_percent{htobe16(10)} {}

    const uint64_t timestamp_ms;
    static constexpr uint8_t nlen = 8;
    const char* name = "handdata";
    const uint32_t temperature_centi_K;
    const uint16_t humidity_deci_percent;

    const reader::Message::time_point_t expected_timestamp{std::chrono::milliseconds{123}};
};

TEST_F(MessageWithDefaults, FromHandCreatedSensorData) {
    std::array<std::byte, ws::timestamp + ws::nlen + nlen + ws::temperature + ws::humidity> data;
    std::memcpy(&data[0], &timestamp_ms, ws::timestamp);
    std::memcpy(&data[ws::timestamp], &nlen, ws::nlen);
    std::memcpy(&data[ws::timestamp + ws::nlen], name, nlen);
    std::memcpy(&data[ws::timestamp + ws::nlen + nlen], &temperature_centi_K, ws::temperature);
    std::memcpy(&data[ws::timestamp + ws::nlen + nlen + ws::temperature],
                &humidity_deci_percent,
                ws::humidity);

    const auto message = reader::Message{asio::buffer(data)};

    EXPECT_EQ(expected_timestamp, message.timestamp());
    EXPECT_EQ("handdata", message.name());
    EXPECT_EQ(0.0f, *message.temperature());
    EXPECT_EQ(1.0f, *message.humidity());
}
TEST_F(MessageWithDefaults, HasTemperatureButNoHumidity) {
    std::array<std::byte, ws::timestamp + ws::nlen + nlen + ws::temperature> data;
    std::memcpy(&data[0], &timestamp_ms, ws::timestamp);
    std::memcpy(&data[ws::timestamp], &nlen, ws::nlen);
    std::memcpy(&data[ws::timestamp + ws::nlen], name, nlen);
    std::memcpy(&data[ws::timestamp + ws::nlen + nlen], &temperature_centi_K, ws::temperature);

    const auto message = reader::Message{asio::buffer(data)};

    EXPECT_EQ(expected_timestamp, message.timestamp());
    EXPECT_EQ("handdata", message.name());
    EXPECT_EQ(0.0f, *message.temperature());
    EXPECT_FALSE(message.humidity());
}

TEST_F(MessageWithDefaults, HasHumidityButNoTemperature) {
    std::array<std::byte, ws::timestamp + ws::nlen + nlen + ws::humidity> data;
    std::memcpy(&data[0], &timestamp_ms, ws::timestamp);
    std::memcpy(&data[ws::timestamp], &nlen, ws::nlen);
    std::memcpy(&data[ws::timestamp + ws::nlen], name, nlen);
    std::memcpy(&data[ws::timestamp + ws::nlen + nlen], &humidity_deci_percent, ws::humidity);

    const auto message = reader::Message{asio::buffer(data)};

    EXPECT_EQ(expected_timestamp, message.timestamp());
    EXPECT_EQ("handdata", message.name());
    EXPECT_FALSE(message.temperature());
    EXPECT_EQ(1.0f, *message.humidity());
}

TEST_F(MessageWithDefaults, BufferTooLarge) {
    std::array<std::byte, ws::timestamp + ws::nlen + nlen + ws::temperature + ws::humidity + 1>
        data;
    std::memcpy(&data[0], &timestamp_ms, ws::timestamp);
    std::memcpy(&data[ws::timestamp], &nlen, ws::nlen);
    std::memcpy(&data[ws::timestamp + ws::nlen], name, nlen);
    std::memcpy(&data[ws::timestamp + ws::nlen + nlen], &temperature_centi_K, ws::temperature);
    std::memcpy(&data[ws::timestamp + ws::nlen + nlen + ws::temperature],
                &humidity_deci_percent,
                ws::humidity);

    EXPECT_THROW(reader::Message{asio::buffer(data)};, reader::bad_message_data);
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
