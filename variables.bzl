CXX_WARN_OPTS = [
    "-Werror",
    "-Wall",
    "-Wextra",
    "-Wnon-virtual-dtor",
    "-Wold-style-cast",
    "-Wcast-align",
    "-Wunused",
    "-Woverloaded-virtual",
    "-Wpedantic",
    "-Wconversion",
    "-Wsign-conversion",
    "-Wdouble-promotion",
    "-Wformat=2",
]

RECORDER_COPTS = ["-Iinclude"] + CXX_WARN_OPTS
