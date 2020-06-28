#include "compat/asio.h"
#include "connection.h"
#include "test/socket.h"

#include "gtest/gtest.h"
#include <iostream>

TEST(Connection, dummy) {
    auto io_context = asio::io_context{};

    auto test_socket = test::socket{io_context, {"address", 12345}};
    auto& inbuf = test_socket.buffer();

    // clang-format off
    constexpr auto message = std::array<uint8_t, 16>{
        0x00, 0x00, 0x00, 0x10, // message length
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // timestamp
        0x03, // nlen
        0xff, 0xff, 0xff // name
    };
    // clang-format on

    inbuf.commit(asio::buffer_copy(inbuf.prepare(message.size()), asio::buffer(message)));

    logger::make_connection(std::move(test_socket));

    EXPECT_EQ(1, io_context.poll_one());
    EXPECT_EQ(1, io_context.poll_one());
}
