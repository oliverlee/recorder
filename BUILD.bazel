load("@generated_compiler_config//:variables.bzl", "RECORDER_DEFAULT_COPTS")

cc_library(
    name = "recorder",
    srcs = ["src/message.cc"],
    hdrs = glob(["include/**/*.h"]),
    copts = RECORDER_DEFAULT_COPTS,
    deps = [
        "@external//:asio",
        "@external//:expected-lite",
        "@external//:json",
    ],
    visibility = ["//tests:__pkg__"],
)

cc_binary(
    name = "reader",
    srcs = ["src/reader.cc"],
    copts = RECORDER_DEFAULT_COPTS,
    deps = [":recorder"],
)

cc_binary(
    name = "logger",
    srcs = ["src/logger.cc"],
    copts = RECORDER_DEFAULT_COPTS,
    deps = [":recorder"],
)
