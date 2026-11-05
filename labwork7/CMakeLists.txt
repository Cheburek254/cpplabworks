cmake_minimum_required(VERSION 3.12)

project(
    labwork7
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build tests" ON)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(FETCHCONTENT_QUIET ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CPR_GENERATE_COVERAGE OFF CACHE BOOL "" FORCE)
set(CPR_CURL_NOSIGNAL ON CACHE BOOL "" FORCE)
set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "" FORCE)
set(HTTP_ONLY ON CACHE BOOL "" FORCE)
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAPS ON CACHE BOOL "" FORCE)

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(ZLIB QUIET)
find_package(OpenSSL QUIET)

include(FetchContent)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.4
    GIT_SHALLOW TRUE
)

set(OLD_EXPORT ${CMAKE_EXPORT_COMPILE_COMMANDS})
set(BACKUP_CXX_TIDY ${CMAKE_CXX_CLANG_TIDY})
set(BACKUP_C_TIDY ${CMAKE_C_CLANG_TIDY})

set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)
set(CMAKE_CXX_CLANG_TIDY "")
set(CMAKE_C_CLANG_TIDY "")

set(OLD_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-everything")

FetchContent_MakeAvailable(json cpr)

set(CMAKE_CXX_FLAGS ${OLD_CMAKE_CXX_FLAGS})
set(CMAKE_CXX_CLANG_TIDY ${BACKUP_CXX_TIDY})
set(CMAKE_C_CLANG_TIDY ${BACKUP_C_TIDY})
set(CMAKE_EXPORT_COMPILE_COMMANDS ${OLD_EXPORT})

include_directories(include)

set(SOURCES
    src/yandex_api.cpp
    src/router.cpp
    src/app.cpp
)

add_library(labwork7_core STATIC ${SOURCES})

target_include_directories(labwork7_core SYSTEM BEFORE PRIVATE 
    $<TARGET_PROPERTY:cpr::cpr,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:nlohmann_json::nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
)
target_include_directories(labwork7_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(labwork7_core PRIVATE 
    $<$<CONFIG:Debug>:BOOST_BIND_GLOBAL_PLACEHOLDERS>
)

if(NOT "$ENV{GITHUB_JOB}" STREQUAL "codestyle")
    target_precompile_headers(labwork7_core PRIVATE
        <nlohmann/json.hpp>
        <cpr/cpr.h>
    )
endif()

target_link_libraries(labwork7_core
    PUBLIC
    cpr::cpr
    nlohmann_json::nlohmann_json
    Threads::Threads
    CURL::libcurl
)

add_executable(labwork7 src/main.cpp)
target_link_libraries(labwork7 PRIVATE labwork7_core)

if(WIN32)
    target_link_libraries(labwork7_core PUBLIC crypt32 bcrypt winhttp ws2_32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(labwork7_core PUBLIC pthread rt)
endif()

if(OPENSSL_FOUND)
    target_link_libraries(labwork7_core PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(labwork7_core PRIVATE -Wall -Wextra)
    target_compile_options(labwork7 PRIVATE -Wall -Wextra)
endif()

if(BUILD_TESTS AND NOT "$ENV{GITHUB_JOB}" STREQUAL "codestyle")
    enable_testing()
    add_subdirectory(tests)
endif()

include(tests/coverage.cmake OPTIONAL)
