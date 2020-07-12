COPTS = ["-Iinclude"]

cc_library(
    name = "recorder",
    srcs = ["src/message.cc"],
    hdrs = glob(["include/**/*.h"]),
    copts = COPTS,
    deps = [
        "@external//:asio",
        "@external//:expected-lite",
        "@external//:json",
    ],
)

cc_binary(
    name = "reader",
    srcs = ["src/reader.cc"],
    deps = [":recorder"],
    copts = COPTS,
)

cc_binary(
    name = "logger",
    srcs = ["src/logger.cc"],
    deps = [":recorder"],
    copts = COPTS,
)
