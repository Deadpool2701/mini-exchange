cmake_minimum_required(VERSION 3.20)

project(MiniExchange
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------------
# Exchange library
# -------------------------

add_library(exchange
    src/exchange/PriceLevel.cpp
)

target_include_directories(exchange
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# -------------------------
# Main executable
# -------------------------

add_executable(mini_exchange
    src/main.cpp
)

target_link_libraries(mini_exchange
    PRIVATE
    exchange
)

# -------------------------
# GoogleTest
# -------------------------

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(googletest)

# -------------------------
# Tests
# -------------------------

enable_testing()

add_executable(unit_tests
    tests/OrderTests.cpp
    tests/PriceLevelTests.cpp
)

target_link_libraries(unit_tests
    PRIVATE
    exchange
    GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(unit_tests)