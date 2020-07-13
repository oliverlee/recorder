load("//:variables.bzl", "COMMON_CXX_WARN_OPTS")
RECORDER_DEFAULT_COPTS = ["-Iinclude"] + COMMON_CXX_WARN_OPTS

config_setting(
    name = "macOS",
    constraint_values = [
        "@platforms//os:macos",
    ]
)

config_setting(
    name = "Linux",
    constraint_values = [
        "@platforms//os:linux",
    ]
)

cc_test(
    name = "test_message",
    size = "small",
    srcs = ["test_message.cc"],
    copts = RECORDER_DEFAULT_COPTS,
    deps = [
        "@googletest//:gtest_main",
        "//:recorder",
    ],
    data = ["//tests:data/testdata.log"],
    linkopts = select({
        "macOS" : [],
        "Linux" : ["-lstdc++fs"],
    }),
)

cc_test(
    name = "test_connection",
    size = "small",
    srcs = ["test_connection.cc"] + glob(["include/test/*.h"]),
    copts = RECORDER_DEFAULT_COPTS + ["-Itests/include"],
    deps = [
        "@googletest//:gtest_main",
        "//:recorder",
    ],
)
