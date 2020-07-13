COMMON_CXX_WARN_OPTS = [
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

CLANG_CXX_WARN_OPTS = [
    "-Wnull-dereference",
]

GCC_CXX_WARN_OPTS = [
    "-Wshadow=compatible-local",
    "-Wlogical-op",
    "-Wuseless-cast",
    "-Wduplicated-cond",
    "-Wduplicated-branches",
    "-Wmisleading-indentation",
]

RECORDER_INCLUDE_DIR_OPT = ["-Iinclude"]
RECORDER_DEFAULT_COPTS = RECORDER_INCLUDE_DIR_OPT + COMMON_CXX_WARN_OPTS + %{compiler_name}_CXX_WARN_OPTS

GCC_MACOS_LIBFS = ["-lstdc++fs"]
CLANG_MACOS_LIBFS = [""]

MACOS_LIBFS = %{compiler_name}_MACOS_LIBFS
