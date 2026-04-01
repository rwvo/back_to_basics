#include <gtest/gtest.h>
#include <sstream>
#include "interpreter.h"

using namespace rocbas;

static std::string run(const std::string& source) {
    std::ostringstream out;
    Interpreter interp(out);
    interp.run(source);
    return out.str();
}

// --- MPI INIT / FINALIZE ---

TEST(MpiInterpreter, MpiInitFinalize) {
    EXPECT_NO_THROW(run(
        "10 MPI INIT\n"
        "20 MPI FINALIZE\n"
        "30 END\n"
    ));
}

// --- MPI RANK / MPI SIZE ---

TEST(MpiInterpreter, MpiRankSingleProcess) {
    EXPECT_EQ(run(
        "10 MPI INIT\n"
        "20 PRINT MPI RANK\n"
        "30 MPI FINALIZE\n"
        "40 END\n"
    ), "0\n");
}

TEST(MpiInterpreter, MpiSizeSingleProcess) {
    EXPECT_EQ(run(
        "10 MPI INIT\n"
        "20 PRINT MPI SIZE\n"
        "30 MPI FINALIZE\n"
        "40 END\n"
    ), "1\n");
}

TEST(MpiInterpreter, MpiRankInExpression) {
    EXPECT_EQ(run(
        "10 MPI INIT\n"
        "20 LET X = MPI RANK + 1\n"
        "30 PRINT X\n"
        "40 MPI FINALIZE\n"
        "50 END\n"
    ), "1\n");
}

TEST(MpiInterpreter, MpiRankBeforeInit) {
    EXPECT_THROW(run(
        "10 PRINT MPI RANK\n"
        "20 END\n"
    ), std::runtime_error);
}

// --- MPI SEND / RECV ---

TEST(MpiInterpreter, MpiSendRecvWholeArray) {
    EXPECT_EQ(run(
        "10 MPI INIT\n"
        "20 DIM A(3), B(3)\n"
        "30 LET A(1) = 10\n"
        "40 LET A(2) = 20\n"
        "50 LET A(3) = 30\n"
        "60 MPI SEND A TO 0 TAG 1\n"
        "70 MPI RECV B FROM 0 TAG 1\n"
        "80 PRINT B(1); B(2); B(3)\n"
        "90 MPI FINALIZE\n"
        "100 END\n"
    ), "102030\n");
}

TEST(MpiInterpreter, MpiSendRecvRange) {
    EXPECT_EQ(run(
        "10 MPI INIT\n"
        "20 DIM A(5), B(5)\n"
        "30 FOR I = 1 TO 5\n"
        "40 LET A(I) = I * 10\n"
        "50 NEXT I\n"
        "60 MPI SEND A(2) THRU A(4) TO 0 TAG 1\n"
        "70 MPI RECV B(2) THRU B(4) FROM 0 TAG 1\n"
        "80 PRINT B(2); B(3); B(4)\n"
        "90 MPI FINALIZE\n"
        "100 END\n"
    ), "203040\n");
}

TEST(MpiInterpreter, MpiSendWithoutInit) {
    EXPECT_THROW(run(
        "10 DIM A(3)\n"
        "20 MPI SEND A TO 0 TAG 1\n"
        "30 END\n"
    ), std::runtime_error);
}

TEST(MpiInterpreter, MpiRecvWithoutInit) {
    EXPECT_THROW(run(
        "10 DIM B(3)\n"
        "20 MPI RECV B FROM 0 TAG 1\n"
        "30 END\n"
    ), std::runtime_error);
}

// --- MPI BARRIER ---

TEST(MpiInterpreter, MpiBarrierSingleProcess) {
    EXPECT_EQ(run(
        "10 MPI INIT\n"
        "20 MPI BARRIER\n"
        "30 PRINT \"after barrier\"\n"
        "40 MPI FINALIZE\n"
        "50 END\n"
    ), "after barrier\n");
}

TEST(MpiInterpreter, MpiBarrierWithoutInit) {
    EXPECT_THROW(run(
        "10 MPI BARRIER\n"
        "20 END\n"
    ), std::runtime_error);
}

// --- Full program ---

TEST(MpiInterpreter, MpiFullProgramSingleProcess) {
    EXPECT_EQ(run(
        "10 MPI INIT\n"
        "20 LET R = MPI RANK\n"
        "30 LET S = MPI SIZE\n"
        "40 PRINT R; S\n"
        "50 DIM A(4)\n"
        "60 FOR I = 1 TO 4\n"
        "70 LET A(I) = I * 100\n"
        "80 NEXT I\n"
        "90 MPI SEND A TO 0 TAG 1\n"
        "100 DIM B(4)\n"
        "110 MPI RECV B FROM 0 TAG 1\n"
        "120 MPI BARRIER\n"
        "130 PRINT B(1); B(4)\n"
        "140 MPI FINALIZE\n"
        "150 END\n"
    ), "01\n100400\n");
}
