include(GoogleTest)

add_executable(taskboard_tests
  task_test.cpp
  task_store_test.cpp
  api_test.cpp
)
target_link_libraries(taskboard_tests PRIVATE taskboard_api GTest::gtest_main)
target_compile_options(taskboard_tests PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>)

gtest_discover_tests(taskboard_tests)
