cmake_minimum_required(VERSION 3.20)
project(taskboard VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TASKBOARD_BUILD_TESTS "Build unit and API tests" ON)

include(FetchContent)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG        v0.18.3
)
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(httplib json)

find_package(Threads REQUIRED)

# ---- core library (no networking, easy to unit test) ----
add_library(taskboard_core
  src/task.cpp
  src/task_store.cpp
)
target_include_directories(taskboard_core PUBLIC include)
target_link_libraries(taskboard_core PUBLIC nlohmann_json::nlohmann_json)
target_compile_definitions(taskboard_core PUBLIC TASKBOARD_VERSION="${PROJECT_VERSION}")

# ---- HTTP API layer ----
add_library(taskboard_api src/api.cpp)
target_link_libraries(taskboard_api PUBLIC taskboard_core httplib::httplib Threads::Threads)

# ---- server executable ----
add_executable(taskboard_server src/main.cpp)
target_link_libraries(taskboard_server PRIVATE taskboard_api)

foreach(tgt taskboard_core taskboard_api taskboard_server)
  target_compile_options(${tgt} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>)
endforeach()

install(TARGETS taskboard_server RUNTIME DESTINATION bin)

# ---- tests ----
if(TASKBOARD_BUILD_TESTS)
  enable_testing()
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  add_subdirectory(tests)
endif()
