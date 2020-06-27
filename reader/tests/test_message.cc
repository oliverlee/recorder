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

namespace {
const auto logfile = (fs::path{__FILE__}.parent_path() / "data/testdata.log").string();
} // namespace

TEST(Message, FromToolGeneratedSensorData) {
    auto sensor_data = std::ifstream{logfile, std::ios::in | std::ios_base::binary | std::ios::ate};
    const auto stream_size = sensor_data.tellg();
    sensor_data.seekg(0);

    const uint32_t message_length = [&sensor_data]() {
        uint32_t temp_be;
        // Just be lazy about type punning here
        sensor_data.read(reinterpret_cast<char*>(&temp_be), sizeof(temp_be));
        return be32toh(temp_be);
    }();

    // Ensure the recorded log contains at least a full message.
    // It is assumed the beginning of the file starts with message length.
    ASSERT_GE(stream_size, message_length);

    const auto payload_length = message_length - 4;
    auto data = std::vector<char>(payload_length);
    sensor_data.read(data.data(), payload_length);

    const auto message = reader::Message{asio::buffer(data)};
    std::cout << message << "\n";

    EXPECT_EQ("testdata", message.name());
}

TEST(Message, FromHandCreatedSensorData) {
    const uint64_t timestamp_ms = htobe64(123);
    const uint8_t nlen = 8;
    const char* name = "handdata";
    const uint32_t temperature_centi_K = htobe32(static_cast<uint32_t>(27315) << 8);
    const uint16_t humidity_deci_percent = htobe16(10);

    std::array<std::byte, 8 + 1 + 8 + 3 + 2> data;
    std::memcpy(&data[0], &timestamp_ms, 8);
    std::memcpy(&data[8], &nlen, 1);
    std::memcpy(&data[9], name, nlen);
    std::memcpy(&data[9 + nlen], &temperature_centi_K, 3);
    std::memcpy(&data[9 + nlen + 3], &humidity_deci_percent, 2);

    const auto message = reader::Message{asio::buffer(data)};
    std::cout << message << "\n";

    const auto expected_timestamp = reader::Message::time_point_t{std::chrono::milliseconds{123}};

    EXPECT_EQ(expected_timestamp, message.timestamp());
    EXPECT_EQ("handdata", message.name());
    EXPECT_EQ(0.0f, *message.temperature());
    EXPECT_EQ(1.0f, *message.humidity());
}
