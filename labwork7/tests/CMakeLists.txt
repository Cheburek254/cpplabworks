find_package(GTest CONFIG REQUIRED)

include_directories(${GTEST_INCLUDE_DIRS})
include_directories(../include)


add_executable(labwork7_tests
    test_main.cpp
    test_yandex_api.cpp
    test_router.cpp
    test_cache.cpp
    test_app.cpp
)

target_link_libraries(labwork7_tests
    PRIVATE
    labwork7_core
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)

target_include_directories(labwork7_tests SYSTEM
    BEFORE
    PRIVATE
        $<TARGET_PROPERTY:GTest::gtest,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:GTest::gmock,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:cpr::cpr,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:nlohmann_json::nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
)

include(GoogleTest)
gtest_discover_tests(labwork7_tests)

if(NOT "$ENV{GITHUB_JOB}" STREQUAL "codestyle")
    target_precompile_headers(labwork7_tests PRIVATE
        <nlohmann/json.hpp>
        <cpr/cpr.h>
        <gtest/gtest.h>
        <gmock/gmock.h>
    )
endif()

target_compile_features(labwork7_tests PRIVATE cxx_std_23)

if(WIN32)
    target_compile_definitions(labwork7_tests PRIVATE _SILENCE_ALL_CXX17_DEPRECATION_WARNING)
endif()
