load("//:variables.bzl", "RECORDER_COPTS")

cc_test(
    name = "test_message",
    size = "small",
    srcs = ["test_message.cc"],
    copts = RECORDER_COPTS,
    deps = [
        "@googletest//:gtest_main",
        "//:recorder",
    ],
    data = ["//tests:data/testdata.log"],
)

cc_test(
    name = "test_connection",
    size = "small",
    srcs = ["test_connection.cc"] + glob(["include/**/*"]),
    copts = RECORDER_COPTS + ["-Itests/include"],
    deps = [
        "@googletest//:gtest_main",
        "//:recorder",
    ],
)
