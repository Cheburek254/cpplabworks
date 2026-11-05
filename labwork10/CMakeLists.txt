cmake_minimum_required(VERSION 3.14)
project(labwork10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(labwork_lib
    src/Database.cpp
    src/Geo.cpp
)
target_include_directories(labwork_lib PUBLIC src)

add_executable(labwork src/main.cpp)
target_link_libraries(labwork PRIVATE labwork_lib)

enable_testing()

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(labwork_tests
    tests/TestString.cpp
    tests/TestList.cpp
    tests/TestSet.cpp
    tests/TestGeo.cpp
    tests/TestGeneral.cpp
)
target_link_libraries(labwork_tests PRIVATE labwork_lib GTest::gtest_main)
gtest_discover_tests(labwork_tests)