#include <gtest/gtest.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"

using namespace rocbas;

static Program parse(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    return parser.parse();
}

template <typename T>
const T& get_stmt(const Program& prog, size_t index) {
    return std::get<T>(prog.lines.at(index)->stmt);
}

// --- GPU DIM ---

TEST(GpuParser, GpuDim) {
    auto prog = parse("10 GPU DIM GA(1024)");
    auto& dim = get_stmt<GpuDimStmt>(prog, 0);
    ASSERT_EQ(dim.arrays.size(), 1u);
    EXPECT_EQ(dim.arrays[0].name, "GA");
    EXPECT_EQ(dim.arrays[0].dimensions.size(), 1u);
}

TEST(GpuParser, GpuDimMultiple) {
    auto prog = parse("10 GPU DIM GA(1024), GB(1024), GC(1024)");
    auto& dim = get_stmt<GpuDimStmt>(prog, 0);
    ASSERT_EQ(dim.arrays.size(), 3u);
    EXPECT_EQ(dim.arrays[0].name, "GA");
    EXPECT_EQ(dim.arrays[1].name, "GB");
    EXPECT_EQ(dim.arrays[2].name, "GC");
}

// --- GPU COPY ---

TEST(GpuParser, GpuCopy) {
    auto prog = parse("10 GPU COPY A TO GA");
    auto& copy = get_stmt<GpuCopyStmt>(prog, 0);
    EXPECT_EQ(copy.src, "A");
    EXPECT_EQ(copy.dst, "GA");
}

// --- GPU FREE ---

TEST(GpuParser, GpuFree) {
    auto prog = parse("10 GPU FREE GA");
    auto& free = get_stmt<GpuFreeStmt>(prog, 0);
    EXPECT_EQ(free.name, "GA");
}

// --- GPU KERNEL ---

TEST(GpuParser, GpuKernel) {
    auto prog = parse(
        "110 GPU KERNEL VECADD(X, Y, Z, N)\n"
        "120   LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "130   IF I < N THEN LET Z(I) = X(I) + Y(I)\n"
        "140 END KERNEL\n"
    );
    ASSERT_EQ(prog.lines.size(), 1u);
    auto& kernel = get_stmt<GpuKernelStmt>(prog, 0);
    EXPECT_EQ(kernel.name, "VECADD");
    ASSERT_EQ(kernel.params.size(), 4u);
    EXPECT_EQ(kernel.params[0], "X");
    EXPECT_EQ(kernel.params[1], "Y");
    EXPECT_EQ(kernel.params[2], "Z");
    EXPECT_EQ(kernel.params[3], "N");
    ASSERT_EQ(kernel.body.size(), 2u);
}

TEST(GpuParser, GpuKernelIntrinsics) {
    // Verify that GPU intrinsics parse correctly inside a kernel body
    auto prog = parse(
        "10 GPU KERNEL TEST(A)\n"
        "20 LET X = THREAD_IDX(1)\n"
        "30 LET Y = BLOCK_IDX(2)\n"
        "40 LET Z = BLOCK_DIM(1) * GRID_DIM(3)\n"
        "50 END KERNEL\n"
    );
    auto& kernel = get_stmt<GpuKernelStmt>(prog, 0);
    EXPECT_EQ(kernel.name, "TEST");
    ASSERT_EQ(kernel.body.size(), 3u);
}

// --- GPU GOSUB ---

TEST(GpuParser, GpuGosub1D) {
    auto prog = parse("10 GPU GOSUB VECADD(GA, GB, GC, 1024) WITH 4 BLOCKS OF 256");
    auto& gosub = get_stmt<GpuGosubStmt>(prog, 0);
    EXPECT_EQ(gosub.kernel_name, "VECADD");
    ASSERT_EQ(gosub.args.size(), 4u);
    ASSERT_EQ(gosub.grid_dims.size(), 1u);
    ASSERT_EQ(gosub.block_dims.size(), 1u);
}

TEST(GpuParser, GpuGosub3D) {
    auto prog = parse("10 GPU GOSUB HEAT(G, N) WITH (4, 4, 1) BLOCKS OF (16, 16, 1)");
    auto& gosub = get_stmt<GpuGosubStmt>(prog, 0);
    EXPECT_EQ(gosub.kernel_name, "HEAT");
    ASSERT_EQ(gosub.args.size(), 2u);
    ASSERT_EQ(gosub.grid_dims.size(), 3u);
    ASSERT_EQ(gosub.block_dims.size(), 3u);
}

// --- Full vecadd program ---

TEST(GpuParser, FullVecaddProgram) {
    auto prog = parse(
        "10 DIM A(1024), B(1024), C(1024)\n"
        "20 FOR I = 1 TO 1024\n"
        "30 LET A(I) = I\n"
        "40 LET B(I) = I * 2\n"
        "50 NEXT I\n"
        "60 GPU DIM GA(1024), GB(1024), GC(1024)\n"
        "70 GPU COPY A TO GA\n"
        "80 GPU COPY B TO GB\n"
        "90 GPU KERNEL VECADD(X, Y, Z, N)\n"
        "100 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "110 IF I < N THEN LET Z(I) = X(I) + Y(I)\n"
        "120 END KERNEL\n"
        "130 GPU GOSUB VECADD(GA, GB, GC, 1024) WITH 4 BLOCKS OF 256\n"
        "140 GPU COPY GC TO C\n"
        "150 PRINT C(1)\n"
        "160 END\n"
    );
    // Count statements: DIM, FOR, LET, LET, NEXT, GPU DIM, GPU COPY, GPU COPY,
    // GPU KERNEL, GPU GOSUB, GPU COPY, PRINT, END = 13
    ASSERT_EQ(prog.lines.size(), 13u);
}
