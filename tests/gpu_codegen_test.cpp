#include <gtest/gtest.h>
#include "lexer.h"
#include "parser.h"
#include "gpu_codegen.h"
#include "gpu_runtime.h"
#include <string>
#include <vector>

using namespace rocbas;

// Helper: parse a multi-line program and extract the GpuKernelStmt
static const GpuKernelStmt& parse_kernel(const std::string& source, Program& prog) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    prog = parser.parse();
    for (const auto& line : prog.lines) {
        if (std::holds_alternative<GpuKernelStmt>(line->stmt)) {
            return std::get<GpuKernelStmt>(line->stmt);
        }
    }
    throw std::runtime_error("No GPU KERNEL found in source");
}

TEST(GpuCodegen, SimpleVecAdd) {
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL VECADD(X, Y, Z, N)\n"
        "20 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I < N THEN LET Z(I) = X(I) + Y(I)\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel);

    // Verify the generated source has the right structure
    EXPECT_NE(source.find("extern \"C\" __global__ void VECADD"), std::string::npos)
        << "Should have extern C global function";
    EXPECT_NE(source.find("double* X"), std::string::npos)
        << "X should be a pointer parameter";
    EXPECT_NE(source.find("double* Y"), std::string::npos)
        << "Y should be a pointer parameter";
    EXPECT_NE(source.find("double* Z"), std::string::npos)
        << "Z should be a pointer parameter";
    EXPECT_NE(source.find("double N"), std::string::npos)
        << "N should be a scalar parameter";
    EXPECT_NE(source.find("blockIdx.x"), std::string::npos)
        << "Should use blockIdx.x";
    EXPECT_NE(source.find("blockDim.x"), std::string::npos)
        << "Should use blockDim.x";
    EXPECT_NE(source.find("threadIdx.x"), std::string::npos)
        << "Should use threadIdx.x";
    // GPU arrays use 0-based indexing (matching HIP convention)
    EXPECT_NE(source.find("[(int)("), std::string::npos)
        << "Array access should cast index to int";
    // Local variable I should be declared
    EXPECT_NE(source.find("double I"), std::string::npos)
        << "Local variable I should be declared";
}

TEST(GpuCodegen, ScalarOnlyKernel) {
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL SCALE(A, N, FACTOR)\n"
        "20 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I < N THEN LET A(I) = A(I) * FACTOR\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel);

    // A is array (used with array access), N and FACTOR are scalars
    EXPECT_NE(source.find("double* A"), std::string::npos);
    EXPECT_NE(source.find("double N"), std::string::npos);
    EXPECT_NE(source.find("double FACTOR"), std::string::npos);
}

TEST(GpuCodegen, 3DIntrinsics) {
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL TEST3D(A)\n"
        "20 LET X = THREAD_IDX(1)\n"
        "30 LET Y = THREAD_IDX(2)\n"
        "40 LET Z = BLOCK_IDX(3)\n"
        "50 LET W = GRID_DIM(1) * BLOCK_DIM(2)\n"
        "60 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel);

    EXPECT_NE(source.find("threadIdx.x"), std::string::npos);
    EXPECT_NE(source.find("threadIdx.y"), std::string::npos);
    EXPECT_NE(source.find("blockIdx.z"), std::string::npos);
    EXPECT_NE(source.find("gridDim.x"), std::string::npos);
    EXPECT_NE(source.find("blockDim.y"), std::string::npos);
}

TEST(GpuCodegen, ArithmeticOperations) {
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL MATH(A, N)\n"
        "20 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I < N THEN LET A(I) = A(I) ^ 2 + 3.14\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel);

    EXPECT_NE(source.find("pow("), std::string::npos)
        << "Exponentiation should use pow()";
    EXPECT_NE(source.find("3.14"), std::string::npos);
}

TEST(GpuCodegen, ComparisonInCondition) {
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL CLAMP(A, N)\n"
        "20 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I < N THEN LET A(I) = A(I) + 1\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel);

    // The condition I < N should produce a comparison
    EXPECT_NE(source.find("if ("), std::string::npos);
    EXPECT_NE(source.find("!= 0.0"), std::string::npos)
        << "Condition should check != 0.0";
}

#ifdef ROCBAS_HAS_HIP
// This test actually compiles the generated source with hiprtc to verify it's valid HIP
TEST(GpuCodegen, GeneratedSourceCompilesWithHiprtc) {
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL VECADD(X, Y, Z, N)\n"
        "20 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I < N THEN LET Z(I) = X(I) + Y(I)\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel);

    // Compile with hiprtc
    GpuRuntime runtime;
    if (!runtime.is_available()) {
        GTEST_SKIP() << "No GPU device available";
    }
    EXPECT_NO_THROW(runtime.compile_kernel("VECADD", source));
}
#endif

// Test the full pipeline: codegen + compile + launch on GPU
#ifdef ROCBAS_HAS_HIP
TEST(GpuCodegen, EndToEndVecAdd) {
    GpuRuntime runtime;
    if (!runtime.is_available()) {
        GTEST_SKIP() << "No GPU device available";
    }

    // Parse a vecadd kernel from BASIC
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL VECADD(X, Y, Z, N)\n"
        "20 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I < N THEN LET Z(I) = X(I) + Y(I)\n"
        "40 END KERNEL\n",
        prog
    );

    // Generate HIP source
    std::string source = generate_kernel_source(kernel);

    // Compile
    runtime.compile_kernel("VECADD", source);

    // Set up data
    const size_t N = 1024;
    runtime.gpu_dim("GX", N);
    runtime.gpu_dim("GY", N);
    runtime.gpu_dim("GZ", N);

    std::vector<double> hx(N), hy(N);
    for (size_t i = 0; i < N; i++) {
        hx[i] = static_cast<double>(i + 1);
        hy[i] = static_cast<double>((i + 1) * 2);
    }

    runtime.gpu_copy_host_to_device("HX", "GX", hx.data(), N);
    runtime.gpu_copy_host_to_device("HY", "GY", hy.data(), N);

    // Launch
    runtime.launch_kernel("VECADD",
                          {"GX", "GY", "GZ", "N"},
                          {1024.0},
                          {4}, {256});

    // Verify
    std::vector<double> result(N);
    runtime.gpu_copy_device_to_host("GZ", "HZ", result.data(), N);

    for (size_t i = 0; i < N; i++) {
        EXPECT_DOUBLE_EQ(result[i], static_cast<double>((i + 1) + (i + 1) * 2))
            << "at index " << i;
    }

    runtime.gpu_free("GX");
    runtime.gpu_free("GY");
    runtime.gpu_free("GZ");
}
#endif
