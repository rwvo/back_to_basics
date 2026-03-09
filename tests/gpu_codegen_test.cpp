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

// --- PRINT in GPU kernels ---

TEST(GpuCodegen, PrintStringLiteral) {
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL HELLO(N)\n"
        "20 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I = 0 THEN PRINT \"Hello from GPU!\"\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel);

    // Single printf with string and trailing newline
    EXPECT_NE(source.find("printf(\"Hello from GPU!\\n\")"), std::string::npos)
        << "Should emit single printf with string and newline";
}

TEST(GpuCodegen, PrintNumericExpression) {
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL DBG(A, N)\n"
        "20 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I < N THEN PRINT A(I)\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel);

    // Single printf with %g format, argument cast to double
    EXPECT_NE(source.find("printf(\"%g\\n\", (double)("), std::string::npos)
        << "Numeric expression should use printf with %g and (double) cast";
}

TEST(GpuCodegen, PrintMixedItems) {
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL INFO(N)\n"
        "20 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I = 0 THEN PRINT \"Thread \"; I; \" of \"; N\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel);

    // Single printf with format string combining all items, args cast to double
    EXPECT_NE(source.find("printf(\"Thread %g of %g\\n\", (double)(I), (double)(N))"), std::string::npos)
        << "Should emit single printf with combined format string and (double) casts";
}

TEST(GpuCodegen, PrintNoTrailingNewline) {
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL DBG(N)\n"
        "20 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I = 0 THEN PRINT \"no newline\";\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel);

    // Trailing semicolon suppresses newline — no \n in format string
    EXPECT_NE(source.find("printf(\"no newline\")"), std::string::npos)
        << "Should emit printf without trailing newline";
    EXPECT_EQ(source.find("printf(\"no newline\\n\")"), std::string::npos)
        << "Trailing semicolon should suppress newline";
}

// --- OPTION GPU BASE ---

TEST(GpuCodegen, GpuBase0Default) {
    // Default gpu_base=0: no offset on intrinsics or array access
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL TEST(A, N)\n"
        "20 LET I = THREAD_IDX(1)\n"
        "30 LET A(I) = I\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel, 0);

    // threadIdx.x without offset
    EXPECT_NE(source.find("threadIdx.x"), std::string::npos);
    EXPECT_EQ(source.find("threadIdx.x + "), std::string::npos)
        << "gpu_base 0 should not offset intrinsics";
    // A[(int)(I)] without subtraction
    EXPECT_NE(source.find("A[(int)(I)]"), std::string::npos)
        << "Default gpu_base 0 should not subtract anything";
}

TEST(GpuCodegen, GpuBase1OffsetsIndices) {
    // gpu_base=1: THREAD_IDX and BLOCK_IDX should be offset by +1,
    // array access should subtract 1
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL TEST(A, N)\n"
        "20 LET I = THREAD_IDX(1)\n"
        "30 LET A(I) = A(I) + 1\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel, 1);

    // THREAD_IDX should be offset
    EXPECT_NE(source.find("(threadIdx.x + 1)"), std::string::npos)
        << "gpu_base 1 should offset THREAD_IDX by +1. Source:\n" << source;
    // Array reads and writes should subtract 1 (inside the cast)
    EXPECT_NE(source.find("A[(int)(I - 1)]"), std::string::npos)
        << "gpu_base 1 should subtract 1 in array access. Source:\n" << source;
}

TEST(GpuCodegen, GpuBase1BlockIdxOffset) {
    // gpu_base=1: BLOCK_IDX offset, BLOCK_DIM unchanged
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL FILL(A, N)\n"
        "20 LET I = (BLOCK_IDX(1) - 1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I <= N THEN LET A(I) = I * 2\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel, 1);

    // BLOCK_IDX should be offset
    EXPECT_NE(source.find("(blockIdx.x + 1)"), std::string::npos)
        << "gpu_base 1 should offset BLOCK_IDX. Source:\n" << source;
    // BLOCK_DIM should NOT be offset (it's a size, not an index)
    EXPECT_NE(source.find("blockDim.x"), std::string::npos);
    EXPECT_EQ(source.find("blockDim.x + 1"), std::string::npos)
        << "BLOCK_DIM should not be offset. Source:\n" << source;
    // THREAD_IDX should be offset
    EXPECT_NE(source.find("(threadIdx.x + 1)"), std::string::npos)
        << "gpu_base 1 should offset THREAD_IDX. Source:\n" << source;
    // Array access should subtract 1 (inside the cast)
    EXPECT_NE(source.find("- 1)]"), std::string::npos)
        << "gpu_base 1 should subtract 1 in array access. Source:\n" << source;
}

TEST(GpuCodegen, GpuBaseHalfOffsetsIndices) {
    // gpu_base=0.5: THREAD_IDX and BLOCK_IDX offset by +0.5,
    // array access subtracts 0.5 inside cast
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL TEST(A, N)\n"
        "20 LET I = THREAD_IDX(1)\n"
        "30 LET A(I) = A(I) + 1\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel, 0.5);

    // THREAD_IDX should be offset by 0.5
    EXPECT_NE(source.find("(threadIdx.x + 0.5)"), std::string::npos)
        << "gpu_base 0.5 should offset THREAD_IDX by +0.5. Source:\n" << source;
    // Array access should subtract 0.5 inside the cast
    EXPECT_NE(source.find("A[(int)(I - 0.5)]"), std::string::npos)
        << "gpu_base 0.5 should subtract 0.5 in array access. Source:\n" << source;
}

TEST(GpuCodegen, GpuBaseHalfBlockDimUnchanged) {
    // gpu_base=0.5: BLOCK_DIM not offset, BLOCK_IDX offset by 0.5
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL FILL(A, N)\n"
        "20 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I <= N THEN LET A(I) = I\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel, 0.5);

    // BLOCK_IDX should be offset
    EXPECT_NE(source.find("(blockIdx.x + 0.5)"), std::string::npos)
        << "gpu_base 0.5 should offset BLOCK_IDX. Source:\n" << source;
    // BLOCK_DIM should NOT be offset
    EXPECT_NE(source.find("blockDim.x"), std::string::npos);
    EXPECT_EQ(source.find("blockDim.x + "), std::string::npos)
        << "BLOCK_DIM should not be offset. Source:\n" << source;
    // THREAD_IDX should be offset
    EXPECT_NE(source.find("(threadIdx.x + 0.5)"), std::string::npos)
        << "gpu_base 0.5 should offset THREAD_IDX. Source:\n" << source;
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

TEST(GpuCodegen, PrintKernelCompilesWithHiprtc) {
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL PRINTTEST(A, N)\n"
        "20 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I = 0 THEN PRINT \"Value: \"; A(I)\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel);

    GpuRuntime runtime;
    if (!runtime.is_available()) {
        GTEST_SKIP() << "No GPU device available";
    }
    EXPECT_NO_THROW(runtime.compile_kernel("PRINTTEST", source));
}

// Full pipeline: codegen + compile + launch on GPU
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

TEST(GpuCodegen, PrintFromMultipleThreads) {
    GpuRuntime runtime;
    if (!runtime.is_available()) {
        GTEST_SKIP() << "No GPU device available";
    }

    // Kernel where 4 threads each print their index using a single printf
    // (use a single PRINT item to avoid interleaving between printf calls)
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL PRINTALL(N)\n"
        "20 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I < N THEN PRINT I\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel);
    runtime.compile_kernel("PRINTALL", source);

    // Launch 4 threads, capture stdout
    testing::internal::CaptureStdout();
    runtime.launch_kernel("PRINTALL",
                          {"N"},
                          {4.0},
                          {1}, {4});
    std::string output = testing::internal::GetCapturedStdout();

    // Each thread prints its index — order is non-deterministic,
    // but all 4 values should appear
    for (int i = 0; i < 4; i++) {
        EXPECT_NE(output.find(std::to_string(i)), std::string::npos)
            << "Expected output from thread " << i << " not found in:\n" << output;
    }
}

TEST(GpuCodegen, PrintMixedFromMultipleThreads) {
    GpuRuntime runtime;
    if (!runtime.is_available()) {
        GTEST_SKIP() << "No GPU device available";
    }

    // Multi-item PRINT from multiple threads — now a single printf per PRINT,
    // so each thread's output is atomic (no interleaving within a line).
    Program prog;
    auto& kernel = parse_kernel(
        "10 GPU KERNEL MIXPRINT(N)\n"
        "20 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "30 IF I < N THEN PRINT \"t\"; I\n"
        "40 END KERNEL\n",
        prog
    );

    std::string source = generate_kernel_source(kernel);
    runtime.compile_kernel("MIXPRINT", source);

    testing::internal::CaptureStdout();
    runtime.launch_kernel("MIXPRINT",
                          {"N"},
                          {4.0},
                          {1}, {4});
    std::string output = testing::internal::GetCapturedStdout();

    // Each thread emits a single atomic printf("t%g\n", I)
    // so "t0", "t1", "t2", "t3" should each appear as intact substrings
    EXPECT_FALSE(output.empty()) << "GPU printf should produce output";
    for (int i = 0; i < 4; i++) {
        std::string expected = "t" + std::to_string(i);
        EXPECT_NE(output.find(expected), std::string::npos)
            << "Expected atomic output \"" << expected << "\" not found in:\n" << output;
    }
}
#endif
