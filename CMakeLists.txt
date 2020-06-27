cmake_minimum_required(VERSION 3.10.2)
project(recorder VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COMMON_CXX_OPTIONS
    -Werror
    -Wall
    -Wextra
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wdouble-promotion
    -Wformat=2
)

set(CLANG_CXX_OPTIONS
    ${COMMON_CXX_OPTIONS}
)

set(GCC_CXX_OPTIONS
    ${COMMON_CXX_OPTIONS}
    -Wlogical-op
    -Wuseless-cast
)


if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(DEFAULT_CXX_OPTIONS ${CLANG_CXX_OPTIONS} -Wnull-dereference)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
        set(GCC_CXX_OPTIONS ${GCC_CXX_OPTIONS} -Wno-shadow)
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6)
        set(GCC_CXX_OPTIONS ${GCC_CXX_OPTIONS}
            -Wduplicated-cond
            -Wduplicated-branches
            -Wmisleading-indentation)
    endif()
    set(DEFAULT_CXX_OPTIONS ${GCC_CXX_OPTIONS})
endif()


# Show this in ccmake
option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    include(cmake/googletest.cmake)
    enable_testing()
endif()

set(ASIO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/asio/asio/include)

# Manually set the include dir so we can ignore warnings in this header
set(JSON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include)

set(RECORDER_EXTERNAL_INCLUDE_DIR
    ${ASIO_INCLUDE_DIR}
    ${JSON_INCLUDE_DIR}
)

find_package(Threads)

add_subdirectory(reader)
add_subdirectory(logger)
