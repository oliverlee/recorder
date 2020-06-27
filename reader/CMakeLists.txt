project(reader)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

add_executable(reader src/reader.cc src/message.cc)
target_include_directories(reader PUBLIC include)
target_include_directories(reader SYSTEM PUBLIC ${RECORDER_EXTERNAL_INCLUDE_DIR})
target_compile_options(reader PRIVATE ${DEFAULT_CXX_OPTIONS})
target_link_libraries(reader Threads::Threads)
