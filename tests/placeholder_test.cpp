#include <gtest/gtest.h>
#include <sstream>
#include "interpreter.h"

// Placeholder tests to verify GTest infrastructure works.
// Real tests will be added in subsequent steps.

TEST(Scaffolding, TestFrameworkWorks) {
    EXPECT_EQ(1 + 1, 2);
}

TEST(Scaffolding, InterpreterRuns) {
    std::ostringstream out;
    rocbas::Interpreter interp(out);
    interp.run("10 PRINT \"HELLO\"\n20 END\n");
    EXPECT_EQ(out.str(), "HELLO\n");
}
