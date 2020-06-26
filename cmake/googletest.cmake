add_subdirectory("${CMAKE_SOURCE_DIR}/external/googletest" "external/googletest")

macro(package_add_test testname)
    add_executable(${testname} ${ARGN})
    target_link_libraries(${testname}
        gtest
        gtest_main)
    add_test(NAME ${testname}
        COMMAND ${testname})
    set_target_properties(${testname}
        PROPERTIES FOLDER tests)
    target_include_directories(${testname}
        PUBLIC ${PROJECT_SOURCE_DIR}/include)
    target_compile_options(${testname}
        PRIVATE ${DEFAULT_CXX_OPTIONS})
endmacro()
