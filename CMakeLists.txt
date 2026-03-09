cmake_minimum_required(VERSION 3.14)
project(rocBAS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- rocBAS interpreter library (shared between executable and tests) ---
add_library(rocbas_lib STATIC
    src/lexer.cpp
    src/parser.cpp
    src/interpreter.cpp
    src/environment.cpp
)
target_include_directories(rocbas_lib PUBLIC src)

# --- rocBAS executable ---
add_executable(rocBAS src/main.cpp)
target_link_libraries(rocBAS PRIVATE rocbas_lib)

# --- Testing with Google Test ---
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(rocbas_tests
    tests/placeholder_test.cpp
    tests/lexer_test.cpp
)
target_link_libraries(rocbas_tests PRIVATE rocbas_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(rocbas_tests)
