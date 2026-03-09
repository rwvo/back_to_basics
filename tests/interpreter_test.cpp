#include <gtest/gtest.h>
#include "interpreter.h"
#include <sstream>

using namespace rocbas;

// Helper: run a BASIC program and capture stdout
static std::string run(const std::string& source) {
    std::ostringstream out;
    Interpreter interp(out);
    interp.run(source);
    return out.str();
}

// --- Basic statements ---

TEST(Interpreter, HelloWorld) {
    EXPECT_EQ(run("10 PRINT \"Hello, World!\"\n20 END\n"),
              "Hello, World!\n");
}

TEST(Interpreter, PrintNumber) {
    EXPECT_EQ(run("10 PRINT 42\n20 END\n"), "42\n");
}

TEST(Interpreter, PrintFloat) {
    EXPECT_EQ(run("10 PRINT 3.14\n20 END\n"), "3.14\n");
}

TEST(Interpreter, PrintExpression) {
    EXPECT_EQ(run("10 PRINT 2 + 3\n20 END\n"), "5\n");
}

TEST(Interpreter, PrintMultipleWithSemicolon) {
    EXPECT_EQ(run("10 PRINT \"X=\"; 42\n20 END\n"), "X=42\n");
}

TEST(Interpreter, PrintTrailingSemicolon) {
    EXPECT_EQ(run("10 PRINT \"hello\";\n20 PRINT \" world\"\n30 END\n"),
              "hello world\n");
}

TEST(Interpreter, EmptyPrint) {
    EXPECT_EQ(run("10 PRINT\n20 END\n"), "\n");
}

TEST(Interpreter, LetAndPrint) {
    EXPECT_EQ(run("10 LET X = 42\n20 PRINT X\n30 END\n"), "42\n");
}

TEST(Interpreter, LetOptional) {
    EXPECT_EQ(run("10 X = 42\n20 PRINT X\n30 END\n"), "42\n");
}

TEST(Interpreter, Rem) {
    // REM should be skipped
    EXPECT_EQ(run("10 REM this is a comment\n20 PRINT \"ok\"\n30 END\n"), "ok\n");
}

TEST(Interpreter, Goto) {
    EXPECT_EQ(run(
        "10 GOTO 30\n"
        "20 PRINT \"skipped\"\n"
        "30 PRINT \"reached\"\n"
        "40 END\n"
    ), "reached\n");
}

TEST(Interpreter, EndStopsExecution) {
    EXPECT_EQ(run(
        "10 PRINT \"before\"\n"
        "20 END\n"
        "30 PRINT \"after\"\n"
    ), "before\n");
}

// --- Arithmetic expressions ---

TEST(Interpreter, Addition) {
    EXPECT_EQ(run("10 PRINT 2 + 3\n20 END\n"), "5\n");
}

TEST(Interpreter, Subtraction) {
    EXPECT_EQ(run("10 PRINT 10 - 3\n20 END\n"), "7\n");
}

TEST(Interpreter, Multiplication) {
    EXPECT_EQ(run("10 PRINT 4 * 5\n20 END\n"), "20\n");
}

TEST(Interpreter, Division) {
    EXPECT_EQ(run("10 PRINT 10 / 4\n20 END\n"), "2.5\n");
}

TEST(Interpreter, Exponentiation) {
    EXPECT_EQ(run("10 PRINT 2 ^ 10\n20 END\n"), "1024\n");
}

TEST(Interpreter, Precedence) {
    EXPECT_EQ(run("10 PRINT 2 + 3 * 4\n20 END\n"), "14\n");
}

TEST(Interpreter, Parentheses) {
    EXPECT_EQ(run("10 PRINT (2 + 3) * 4\n20 END\n"), "20\n");
}

TEST(Interpreter, UnaryMinus) {
    EXPECT_EQ(run("10 PRINT -5\n20 END\n"), "-5\n");
}

// --- String operations ---

TEST(Interpreter, StringVariable) {
    EXPECT_EQ(run("10 LET A$ = \"hello\"\n20 PRINT A$\n30 END\n"), "hello\n");
}

TEST(Interpreter, StringConcatenation) {
    EXPECT_EQ(run("10 LET A$ = \"hello\" + \" world\"\n20 PRINT A$\n30 END\n"),
              "hello world\n");
}

// --- Variable scoping ---

TEST(Interpreter, MultipleVariables) {
    EXPECT_EQ(run(
        "10 LET X = 10\n"
        "20 LET Y = 20\n"
        "30 PRINT X + Y\n"
        "40 END\n"
    ), "30\n");
}

TEST(Interpreter, VariableReassignment) {
    EXPECT_EQ(run(
        "10 LET X = 10\n"
        "20 LET X = 20\n"
        "30 PRINT X\n"
        "40 END\n"
    ), "20\n");
}

// --- Integer display ---

TEST(Interpreter, IntegerDisplayedWithoutDecimal) {
    // Whole numbers should display without .0
    EXPECT_EQ(run("10 PRINT 42\n20 END\n"), "42\n");
    EXPECT_EQ(run("10 PRINT 2 + 3\n20 END\n"), "5\n");
}
