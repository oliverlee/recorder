add_executable(reader reader.cc message.cc)
target_include_directories(reader PRIVATE ${RECORDER_INCLUDE_DIR})
target_include_directories(reader SYSTEM PRIVATE ${RECORDER_EXTERNAL_INCLUDE_DIR})
target_compile_options(reader PRIVATE ${DEFAULT_CXX_OPTIONS})
target_link_libraries(reader PRIVATE Threads::Threads)
set_target_properties(reader PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)
