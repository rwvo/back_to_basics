#include <gtest/gtest.h>
#include "mpi_runtime.h"

using namespace rocbas;

TEST(MpiRuntime, Construction) {
    MpiRuntime runtime;
    EXPECT_FALSE(runtime.is_available());
}

TEST(MpiRuntime, InitSetsAvailable) {
    MpiRuntime runtime;
    runtime.init();
    EXPECT_TRUE(runtime.is_available());
    runtime.finalize();
}

TEST(MpiRuntime, SingleProcessRankAndSize) {
    MpiRuntime runtime;
    runtime.init();
    EXPECT_EQ(runtime.rank(), 0);
    EXPECT_EQ(runtime.size(), 1);
    runtime.finalize();
}

TEST(MpiRuntime, BarrierSingleProcess) {
    MpiRuntime runtime;
    runtime.init();
    EXPECT_NO_THROW(runtime.barrier());
    runtime.finalize();
}

TEST(MpiRuntime, DoubleInitThrows) {
    MpiRuntime runtime;
    runtime.init();
    EXPECT_THROW(runtime.init(), std::runtime_error);
    runtime.finalize();
}

TEST(MpiRuntime, FinalizeWithoutInitThrows) {
    MpiRuntime runtime;
    EXPECT_THROW(runtime.finalize(), std::runtime_error);
}

TEST(MpiRuntime, RankBeforeInitThrows) {
    MpiRuntime runtime;
    EXPECT_THROW(runtime.rank(), std::runtime_error);
}

TEST(MpiRuntime, SendRecvSelfLoopback) {
    MpiRuntime runtime;
    runtime.init();
    std::vector<double> send_data = {1.0, 2.0, 3.0};
    std::vector<double> recv_data(3, 0.0);
    runtime.send(send_data.data(), 3, 0, 42);
    runtime.recv(recv_data.data(), 3, 0, 42);
    EXPECT_DOUBLE_EQ(recv_data[0], 1.0);
    EXPECT_DOUBLE_EQ(recv_data[1], 2.0);
    EXPECT_DOUBLE_EQ(recv_data[2], 3.0);
    runtime.finalize();
}

TEST(MpiRuntime, RecvWithoutMatchingTagThrows) {
    MpiRuntime runtime;
    runtime.init();
    std::vector<double> send_data = {1.0};
    std::vector<double> recv_data(1, 0.0);
    runtime.send(send_data.data(), 1, 0, 1);
    EXPECT_THROW(runtime.recv(recv_data.data(), 1, 0, 99), std::runtime_error);
    runtime.finalize();
}
