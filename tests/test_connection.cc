#include "compat/asio.h"
#include "connection.h"
#include "test/socket.h"

#include "gtest/gtest.h"
#include <sstream>
#include <string_view>

namespace {

// These are defined at file scope as they need to outlive any connections
auto out = std::stringstream{};
auto err = std::stringstream{};

class Connection : public ::testing::Test {
  protected:
    auto SetUp() -> void override {
        out.str("");
        err.str("");
    }

    auto push_data(asio::const_buffer data) -> void {
        auto& input = socket.buffer();
        input.commit(asio::buffer_copy(input.prepare(data.size()), data));
    }

    static constexpr unsigned short test_port = 12345;
    const char* test_address = "test_address";

    asio::io_context ioc;
    test::socket socket = {ioc, {test_address, test_port}};
};

} // namespace

TEST_F(Connection, ConnectionCreated) {
    logger::make_connection(std::move(socket), out, err);

    EXPECT_EQ("", out.str());
    EXPECT_EQ("[test_address:12345] Established connection\n", err.str());
    err.str("");

    EXPECT_EQ(0, ioc.poll_one());
}

TEST_F(Connection, ReadHeader) {
    // clang-format off
    constexpr auto message = std::array<uint8_t, 4>{
        0x00, 0x00, 0x00, 0x10 // message length
    };
    // clang-format on

    push_data(asio::buffer(message));
    logger::make_connection(std::move(socket), out, err);

    // clear connection message
    err.str("");

    // Process the first handler - async_read_header
    EXPECT_EQ(1, ioc.poll_one());
    EXPECT_EQ("", out.str());
    EXPECT_EQ("", err.str());
}

TEST_F(Connection, ReadPayload) {
    constexpr auto n = 16;
    // clang-format off
    constexpr auto message = std::array<uint8_t, n>{
        0x00, 0x00, 0x00, n, // message length
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // timestamp
        0x03, // nlen
        'a', 'b', 'c' // name
    };
    // clang-format on

    push_data(asio::buffer(message));
    logger::make_connection(std::move(socket), out, err);

    // clear connection message and process first handler
    err.str("");
    EXPECT_EQ(1, ioc.poll_one());

    // Process the second handler - async_read_payload
    EXPECT_EQ(1, ioc.poll_one());
    EXPECT_EQ("", err.str());

    // save the copy created by `str()` so the view is valid
    const auto out_str = out.str();
    const auto out_view = std::string_view(out_str);

    // JSON fields are unordered so this is annoying to compare
    // We drop the time hour and timezone since we don't know where the CI machines are located
    // Message decoding is mostly handled by test_message.cc
    static constexpr auto npos = std::string_view::npos;
    EXPECT_NE(npos, out_view.find(std::string_view{"    \"name\": \"abc\","}));
    EXPECT_NE(npos, out_view.find(std::string_view{"    \"timestamp\": \"1970-01-01T0"}));
    EXPECT_NE(npos, out_view.find(std::string_view{":00:00.000"}));
    EXPECT_EQ(npos, out_view.find(std::string_view{"temperature"}));
    EXPECT_EQ(npos, out_view.find(std::string_view{"humidity"}));
}

TEST_F(Connection, InvalidUtf8StringName) {
    constexpr auto n = 16;
    // clang-format off
    constexpr auto message = std::array<uint8_t, n>{
        0x00, 0x00, 0x00, n, // message length
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // timestamp
        0x03, // nlen
        0xff, 0xff, 0xff // name
    };
    // clang-format on

    push_data(asio::buffer(message));
    logger::make_connection(std::move(socket), out, err);

    err.str("");

    EXPECT_EQ(1, ioc.poll_one());
    EXPECT_EQ(1, ioc.poll_one());

    EXPECT_EQ("", out.str());
    EXPECT_EQ("[test_address:12345] Unable to decode string: invalid UTF-8 byte at index 0: 0xFF\n",
              err.str());
}

TEST_F(Connection, InvalidMessageLength) {
    constexpr auto n = 7;
    // clang-format off
    constexpr auto message = std::array<uint8_t, n>{
        0x00, 0x00, 0x00, n, // message length
        0x00, 0x00, 0x00 // invalid payload
    };
    // clang-format on

    push_data(asio::buffer(message));
    logger::make_connection(std::move(socket), out, err);

    EXPECT_EQ("", out.str());
    EXPECT_EQ("[test_address:12345] Established connection\n", err.str());
    err.str("");

    EXPECT_EQ(1, ioc.poll_one());
    EXPECT_EQ("", out.str());
    EXPECT_EQ("", err.str());

    EXPECT_EQ(1, ioc.poll_one());
    EXPECT_EQ("", out.str());
    EXPECT_EQ("[test_address:12345] Unable to decode message: `wire_data` is smaller than the "
              "message minimum size.\n",
              err.str());
}

TEST_F(Connection, UnusedBytes_NoTemperatureOrHumidity) {
    constexpr auto n = 17;
    // clang-format off
    constexpr auto message = std::array<uint8_t, n>{
        0x00, 0x00, 0x00, n, // message length
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // timestamp
        0x03, // nlen
        'a', 'b', 'c', // name
        0x00 // unused (too small for temperature or humidity)
    };
    // clang-format on

    push_data(asio::buffer(message));
    logger::make_connection(std::move(socket), out, err);

    err.str("");
    EXPECT_EQ(1, ioc.poll_one());
    EXPECT_EQ(1, ioc.poll_one());

    EXPECT_EQ("", out.str());
    EXPECT_EQ("[test_address:12345] Unable to decode message: `wire_data` contains unused bytes "
              "after message decode.\n",
              err.str());
}

TEST_F(Connection, UnusedBytes_WithTemperatureAndHumidity) {
    constexpr auto n = 22;
    // clang-format off
    constexpr auto message = std::array<uint8_t, n>{
        0x00, 0x00, 0x00, n, // message length
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // timestamp
        0x03, // nlen
        'a', 'b', 'c', // name
        0x01, 0x02, 0x03, // temperature
        0x02, 0x03, // humidity
        0x00 // unused
    };
    // clang-format on

    push_data(asio::buffer(message));
    logger::make_connection(std::move(socket), out, err);

    err.str("");
    EXPECT_EQ(1, ioc.poll_one());
    EXPECT_EQ(1, ioc.poll_one());

    EXPECT_EQ("", out.str());
    EXPECT_EQ("[test_address:12345] Unable to decode message: `wire_data` contains unused bytes "
              "after message decode.\n",
              err.str());
}
