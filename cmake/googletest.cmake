add_subdirectory("${CMAKE_SOURCE_DIR}/external/googletest" "external/googletest")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    if (UNIX)
        if (APPLE)
            if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
                set(libfs c++fs)
            endif()
        else()
            set(libfs stdc++fs)
        endif()
    endif()
else()
    # GCC
    set(libfs stdc++fs)
endif()

macro(package_add_test testname)
    add_executable(${testname} ${ARGN})

    set_target_properties(${testname}
        PROPERTIES FOLDER tests)

    add_test(NAME ${testname}
        COMMAND ${testname})

    target_include_directories(${testname}
        PUBLIC ${PROJECT_SOURCE_DIR}/include)
    target_include_directories(${testname}
        SYSTEM PUBLIC ${RECORDER_EXTERNAL_INCLUDE_DIR})

    target_compile_options(${testname}
        PRIVATE ${DEFAULT_CXX_OPTIONS})

    target_link_libraries(${testname}
        ${libfs}
        gtest
        gtest_main)
endmacro()
