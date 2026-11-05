cmake_minimum_required(VERSION 3.14)

set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "Minimum policy version" FORCE)

project(TaskScheduler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)

set(CPR_CURL_USE_LIBPSL OFF CACHE BOOL "Disable PSL in CURL" FORCE)

FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(TaskSchedulerLib INTERFACE)
target_include_directories(TaskSchedulerLib INTERFACE include)

add_executable(app src/app.cpp)
target_link_libraries(app PRIVATE TaskSchedulerLib cpr::cpr nlohmann_json::nlohmann_json)

enable_testing()
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.12.1
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(tests tests/tests.cpp)
target_link_libraries(tests PRIVATE TaskSchedulerLib gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)