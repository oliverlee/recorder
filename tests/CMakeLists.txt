package_add_test(test_message test_message.cc
    ${PROJECT_SOURCE_DIR}/src/message.cc
)

package_add_test(test_connection test_connection.cc
    ${PROJECT_SOURCE_DIR}/src/message.cc
)
target_include_directories(test_connection PRIVATE include)
