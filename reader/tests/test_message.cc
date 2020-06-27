#include "gtest/gtest.h"
#include <fstream>
#include <string>

#ifdef __APPLE__
#include "compat/endian.h"
#else
#include <endian.h>
#endif

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

TEST(Message, FromSensorData) {
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
}
