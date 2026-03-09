#include <gtest/gtest.h>
#include "gpu_runtime.h"
#include <vector>

using namespace rocbas;

// --- Tests that work on any build (HIP or CPU-only) ---

TEST(GpuRuntime, Construction) {
    // Should not throw, even without a GPU
    GpuRuntime runtime;
    // is_available() returns true only if a GPU is present
    (void)runtime.is_available();
}

// --- GPU-specific tests (skipped if no GPU) ---

class GpuRuntimeTest : public ::testing::Test {
protected:
    GpuRuntime runtime;

    void SetUp() override {
        if (!runtime.is_available()) {
            GTEST_SKIP() << "No GPU device available";
        }
    }
};

TEST_F(GpuRuntimeTest, DeviceName) {
    std::string name = runtime.device_name();
    EXPECT_FALSE(name.empty());
}

TEST_F(GpuRuntimeTest, GpuDimAndFree) {
    runtime.gpu_dim("GA", 1024);
    EXPECT_EQ(runtime.get_device_array_size("GA"), 1024u);
    EXPECT_NE(runtime.get_device_ptr("GA"), nullptr);
    runtime.gpu_free("GA");
}

TEST_F(GpuRuntimeTest, GpuDimDuplicateThrows) {
    runtime.gpu_dim("GA", 1024);
    EXPECT_THROW(runtime.gpu_dim("GA", 1024), std::runtime_error);
    runtime.gpu_free("GA");
}

TEST_F(GpuRuntimeTest, GpuFreeUnknownThrows) {
    EXPECT_THROW(runtime.gpu_free("NONEXISTENT"), std::runtime_error);
}

TEST_F(GpuRuntimeTest, CopyRoundTrip) {
    const size_t N = 256;
    runtime.gpu_dim("GA", N);

    // Prepare host data
    std::vector<double> host_data(N);
    for (size_t i = 0; i < N; i++) {
        host_data[i] = static_cast<double>(i + 1);
    }

    // Copy to device
    runtime.gpu_copy_host_to_device("HOST", "GA", host_data.data(), N);

    // Copy back to host
    std::vector<double> result(N, 0.0);
    runtime.gpu_copy_device_to_host("GA", "HOST", result.data(), N);

    // Verify
    for (size_t i = 0; i < N; i++) {
        EXPECT_DOUBLE_EQ(result[i], static_cast<double>(i + 1));
    }

    runtime.gpu_free("GA");
}

TEST_F(GpuRuntimeTest, CopyToUnknownDeviceArrayThrows) {
    std::vector<double> data(10, 1.0);
    EXPECT_THROW(
        runtime.gpu_copy_host_to_device("HOST", "NONEXISTENT", data.data(), 10),
        std::runtime_error
    );
}

TEST_F(GpuRuntimeTest, CopyFromUnknownDeviceArrayThrows) {
    std::vector<double> data(10);
    EXPECT_THROW(
        runtime.gpu_copy_device_to_host("NONEXISTENT", "HOST", data.data(), 10),
        std::runtime_error
    );
}

TEST_F(GpuRuntimeTest, CompileSimpleKernel) {
    std::string source = R"(
extern "C" __global__ void fill_42(double* A, double N) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < (int)N) {
        A[i] = 42.0;
    }
}
)";
    EXPECT_NO_THROW(runtime.compile_kernel("fill_42", source));
}

TEST_F(GpuRuntimeTest, CompileInvalidKernelThrows) {
    std::string bad_source = "this is not valid HIP code!!!";
    EXPECT_THROW(runtime.compile_kernel("bad_kernel", bad_source), std::runtime_error);
}

TEST_F(GpuRuntimeTest, LaunchKernel) {
    // Compile a simple fill kernel
    std::string source = R"(
extern "C" __global__ void fill_val(double* A, double N) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < (int)N) {
        A[i] = 42.0;
    }
}
)";
    runtime.compile_kernel("fill_val", source);

    // Allocate device memory
    const size_t N = 256;
    runtime.gpu_dim("GA", N);

    // Launch: GA is a device array, 256.0 is a scalar
    runtime.launch_kernel("fill_val",
                          {"GA", "N"},        // arg names
                          {256.0},             // scalar args
                          {1},                 // grid: 1 block
                          {256});              // block: 256 threads

    // Copy back and verify
    std::vector<double> result(N, 0.0);
    runtime.gpu_copy_device_to_host("GA", "HOST", result.data(), N);

    for (size_t i = 0; i < N; i++) {
        EXPECT_DOUBLE_EQ(result[i], 42.0) << "at index " << i;
    }

    runtime.gpu_free("GA");
}

TEST_F(GpuRuntimeTest, VecAdd) {
    // Compile vecadd kernel
    std::string source = R"(
extern "C" __global__ void vecadd(double* X, double* Y, double* Z, double N) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < (int)N) {
        Z[i] = X[i] + Y[i];
    }
}
)";
    runtime.compile_kernel("vecadd", source);

    const size_t N = 1024;
    runtime.gpu_dim("GX", N);
    runtime.gpu_dim("GY", N);
    runtime.gpu_dim("GZ", N);

    // Prepare host data
    std::vector<double> hx(N), hy(N);
    for (size_t i = 0; i < N; i++) {
        hx[i] = static_cast<double>(i + 1);
        hy[i] = static_cast<double>((i + 1) * 2);
    }

    // Copy to device
    runtime.gpu_copy_host_to_device("HX", "GX", hx.data(), N);
    runtime.gpu_copy_host_to_device("HY", "GY", hy.data(), N);

    // Launch with 4 blocks of 256 threads
    runtime.launch_kernel("vecadd",
                          {"GX", "GY", "GZ", "N"},
                          {1024.0},
                          {4},
                          {256});

    // Copy back and verify
    std::vector<double> result(N, 0.0);
    runtime.gpu_copy_device_to_host("GZ", "HZ", result.data(), N);

    for (size_t i = 0; i < N; i++) {
        double expected = static_cast<double>((i + 1) + (i + 1) * 2);
        EXPECT_DOUBLE_EQ(result[i], expected) << "at index " << i;
    }

    runtime.gpu_free("GX");
    runtime.gpu_free("GY");
    runtime.gpu_free("GZ");
}

TEST_F(GpuRuntimeTest, LaunchUncompiledKernelThrows) {
    EXPECT_THROW(
        runtime.launch_kernel("nonexistent", {}, {}, {1}, {1}),
        std::runtime_error
    );
}

TEST_F(GpuRuntimeTest, AutoCleanupOnDestruction) {
    // Create a temporary runtime, allocate memory, let it destruct
    {
        GpuRuntime tmp;
        if (tmp.is_available()) {
            tmp.gpu_dim("TEMP", 1024);
            // Destructor should free this without error
        }
    }
    // If we get here without crash/leak, the test passes
    SUCCEED();
}
