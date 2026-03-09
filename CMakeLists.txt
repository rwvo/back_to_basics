cmake_minimum_required(VERSION 3.14)
project(rocBAS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Optional HIP/GPU support ---
option(ROCBAS_ENABLE_HIP "Enable HIP/GPU support" ON)

set(ROCBAS_GPU_SOURCES src/gpu_runtime.cpp src/gpu_codegen.cpp)

if(ROCBAS_ENABLE_HIP)
    # Try to find HIP
    list(APPEND CMAKE_PREFIX_PATH "/opt/rocm" "/opt/rocm/lib/cmake")
    find_package(hip QUIET)
    if(hip_FOUND)
        message(STATUS "HIP found — GPU support enabled")
        set(ROCBAS_HAS_HIP TRUE)
    else()
        message(STATUS "HIP not found — building CPU-only")
        set(ROCBAS_HAS_HIP FALSE)
    endif()
else()
    message(STATUS "HIP disabled by user — building CPU-only")
    set(ROCBAS_HAS_HIP FALSE)
endif()

# --- rocBAS interpreter library (shared between executable and tests) ---
add_library(rocbas_lib STATIC
    src/lexer.cpp
    src/parser.cpp
    src/interpreter.cpp
    src/environment.cpp
    ${ROCBAS_GPU_SOURCES}
)
target_include_directories(rocbas_lib PUBLIC src)

if(ROCBAS_HAS_HIP)
    target_compile_definitions(rocbas_lib PUBLIC ROCBAS_HAS_HIP)
    target_link_libraries(rocbas_lib PUBLIC hip::host)
    # Link hiprtc for runtime compilation
    find_library(HIPRTC_LIB hiprtc PATHS "/opt/rocm/lib" "/opt/rocm-7.1.0/lib")
    if(HIPRTC_LIB)
        target_link_libraries(rocbas_lib PUBLIC ${HIPRTC_LIB})
    endif()
endif()

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
    tests/ast_test.cpp
    tests/parser_test.cpp
    tests/interpreter_test.cpp
    tests/gpu_parser_test.cpp
    tests/gpu_runtime_test.cpp
    tests/gpu_codegen_test.cpp
    tests/gpu_integration_test.cpp
)
target_link_libraries(rocbas_tests PRIVATE rocbas_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(rocbas_tests)
