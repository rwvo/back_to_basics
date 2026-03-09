#include <gtest/gtest.h>
#include "interpreter.h"
#include "gpu_runtime.h"
#include <sstream>

using namespace rocbas;

#ifdef ROCBAS_HAS_HIP

class GpuIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        GpuRuntime probe;
        if (!probe.is_available()) {
            GTEST_SKIP() << "No GPU device available";
        }
    }
};

TEST_F(GpuIntegrationTest, VecAddProgram) {
    std::ostringstream out;
    std::istringstream in;
    Interpreter interp(out, in);

    interp.run(
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
        "160 PRINT C(512)\n"
        "170 PRINT C(1024)\n"
        "180 GPU FREE GA\n"
        "190 GPU FREE GB\n"
        "200 GPU FREE GC\n"
        "210 END\n"
    );

    // C(1) = A(1) + B(1) = 1 + 2 = 3  → but thread 0 writes to index 0, C(1) is index 0
    // Wait — BASIC C(1) is index 0 in internal storage
    // GPU thread 0: I=0, Z[0] = X[0] + Y[0] = 1 + 2 = 3
    // When copied back, C(1) = element 0 = 3
    // C(512): thread 511: I=511, Z[511] = X[511] + Y[511] = 512 + 1024 = 1536
    // C(1024): thread 1023: I=1023, Z[1023] = X[1023] + Y[1023] = 1024 + 2048 = 3072
    EXPECT_EQ(out.str(), "3\n1536\n3072\n");
}

TEST_F(GpuIntegrationTest, ScaleKernel) {
    std::ostringstream out;
    std::istringstream in;
    Interpreter interp(out, in);

    interp.run(
        "10 DIM A(256)\n"
        "20 FOR I = 1 TO 256\n"
        "30 LET A(I) = I\n"
        "40 NEXT I\n"
        "50 GPU DIM GA(256)\n"
        "60 GPU COPY A TO GA\n"
        "70 GPU KERNEL SCALE(X, N, FACTOR)\n"
        "80 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)\n"
        "90 IF I < N THEN LET X(I) = X(I) * FACTOR\n"
        "100 END KERNEL\n"
        "110 GPU GOSUB SCALE(GA, 256, 10) WITH 1 BLOCKS OF 256\n"
        "120 GPU COPY GA TO A\n"
        "130 PRINT A(1)\n"
        "140 PRINT A(256)\n"
        "150 GPU FREE GA\n"
        "160 END\n"
    );

    // A(1) = 1 * 10 = 10
    // A(256) = 256 * 10 = 2560
    EXPECT_EQ(out.str(), "10\n2560\n");
}

TEST_F(GpuIntegrationTest, GpuKernelNotDefined) {
    std::ostringstream out;
    std::istringstream in;
    Interpreter interp(out, in);

    EXPECT_THROW(
        interp.run(
            "10 GPU DIM GA(1024)\n"
            "20 GPU GOSUB NONEXISTENT(GA, 1024) WITH 1 BLOCKS OF 256\n"
            "30 END\n"
        ),
        std::runtime_error
    );
}

#endif // ROCBAS_HAS_HIP
