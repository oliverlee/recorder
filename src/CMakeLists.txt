macro(recorder_add_executable name)
    add_executable(${name} ${ARGN})

    target_include_directories(${name}
        PRIVATE ${RECORDER_INCLUDE_DIR})

    target_include_directories(${name}
        SYSTEM PRIVATE ${RECORDER_EXTERNAL_INCLUDE_DIR})

    target_compile_options(${name}
        PRIVATE ${DEFAULT_CXX_OPTIONS})

    target_link_libraries(${name}
        PRIVATE Threads::Threads)

    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
    )
endmacro()

recorder_add_executable(reader reader.cc)

recorder_add_executable(logger logger.cc message.cc)
