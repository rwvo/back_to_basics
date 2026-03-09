#include <gtest/gtest.h>
#include "interpreter.h"

// Placeholder tests to verify GTest infrastructure works.
// Real tests will be added in subsequent steps.

TEST(Scaffolding, TestFrameworkWorks) {
    EXPECT_EQ(1 + 1, 2);
}

TEST(Scaffolding, InterpreterThrowsUnimplemented) {
    // Verify that the interpreter stub throws "not yet implemented".
    // This test will be replaced once the interpreter is functional.
    rocbas::Interpreter interp;
    EXPECT_THROW(interp.run("10 PRINT \"HELLO\""), std::runtime_error);
}
