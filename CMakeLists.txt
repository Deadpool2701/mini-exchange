cmake_minimum_required(VERSION 3.20)

project(MiniExchange
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(googletest)

add_executable(mini_exchange
    src/main.cpp
)

target_include_directories(mini_exchange
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

enable_testing()

add_executable(unit_tests
    tests/OrderTests.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        GTest::gtest_main
)

target_include_directories(unit_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_discover_tests(unit_tests)