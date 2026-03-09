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

// --- Control flow: IF/THEN ---

TEST(Interpreter, IfThenTrue) {
    EXPECT_EQ(run(
        "10 LET X = 10\n"
        "20 IF X > 5 THEN PRINT \"big\"\n"
        "30 END\n"
    ), "big\n");
}

TEST(Interpreter, IfThenFalse) {
    EXPECT_EQ(run(
        "10 LET X = 3\n"
        "20 IF X > 5 THEN PRINT \"big\"\n"
        "30 PRINT \"done\"\n"
        "40 END\n"
    ), "done\n");
}

TEST(Interpreter, IfThenGoto) {
    EXPECT_EQ(run(
        "10 LET X = 10\n"
        "20 IF X > 5 THEN 40\n"
        "30 PRINT \"skipped\"\n"
        "40 PRINT \"reached\"\n"
        "50 END\n"
    ), "reached\n");
}

TEST(Interpreter, IfWithAnd) {
    EXPECT_EQ(run(
        "10 LET X = 10\n"
        "20 LET Y = 20\n"
        "30 IF X > 5 AND Y > 15 THEN PRINT \"both\"\n"
        "40 END\n"
    ), "both\n");
}

TEST(Interpreter, IfWithOr) {
    EXPECT_EQ(run(
        "10 LET X = 3\n"
        "20 IF X > 5 OR X < 5 THEN PRINT \"one\"\n"
        "30 END\n"
    ), "one\n");
}

// --- Control flow: FOR/NEXT ---

TEST(Interpreter, ForNextBasic) {
    EXPECT_EQ(run(
        "10 FOR I = 1 TO 5\n"
        "20 PRINT I;\n"
        "30 NEXT I\n"
        "40 END\n"
    ), "12345");
}

TEST(Interpreter, ForNextWithStep) {
    EXPECT_EQ(run(
        "10 FOR I = 0 TO 10 STEP 2\n"
        "20 PRINT I;\n"
        "30 NEXT I\n"
        "40 END\n"
    ), "0246810");
}

TEST(Interpreter, ForNextCountdown) {
    EXPECT_EQ(run(
        "10 FOR I = 5 TO 1 STEP -1\n"
        "20 PRINT I;\n"
        "30 NEXT I\n"
        "40 END\n"
    ), "54321");
}

TEST(Interpreter, ForNextSkip) {
    // Loop where start > end with positive step should not execute body
    EXPECT_EQ(run(
        "10 FOR I = 10 TO 1\n"
        "20 PRINT \"never\"\n"
        "30 NEXT I\n"
        "40 PRINT \"done\"\n"
        "50 END\n"
    ), "done\n");
}

TEST(Interpreter, NestedForLoops) {
    EXPECT_EQ(run(
        "10 FOR I = 1 TO 2\n"
        "20 FOR J = 1 TO 3\n"
        "30 PRINT I * 10 + J;\n"
        "40 NEXT J\n"
        "50 NEXT I\n"
        "60 END\n"
    ), "111213212223");
}

// --- Control flow: GOSUB/RETURN ---

TEST(Interpreter, GosubReturn) {
    EXPECT_EQ(run(
        "10 PRINT \"before\"\n"
        "20 GOSUB 100\n"
        "30 PRINT \"after\"\n"
        "40 END\n"
        "100 PRINT \"sub\"\n"
        "110 RETURN\n"
    ), "before\nsub\nafter\n");
}

TEST(Interpreter, GosubNested) {
    EXPECT_EQ(run(
        "10 GOSUB 100\n"
        "20 END\n"
        "100 PRINT \"A\"\n"
        "110 GOSUB 200\n"
        "120 PRINT \"C\"\n"
        "130 RETURN\n"
        "200 PRINT \"B\"\n"
        "210 RETURN\n"
    ), "A\nB\nC\n");
}

TEST(Interpreter, ReturnWithoutGosub) {
    EXPECT_THROW(run("10 RETURN\n"), std::runtime_error);
}

// --- Fibonacci program ---

TEST(Interpreter, Fibonacci) {
    EXPECT_EQ(run(
        "10 LET A = 0\n"
        "20 LET B = 1\n"
        "30 FOR I = 1 TO 10\n"
        "40 PRINT A;\n"
        "50 LET C = A + B\n"
        "60 LET A = B\n"
        "70 LET B = C\n"
        "80 NEXT I\n"
        "90 END\n"
    ), "0112358132134");
}

// --- Countdown with GOTO loop ---

TEST(Interpreter, CountdownWithGoto) {
    EXPECT_EQ(run(
        "10 LET X = 5\n"
        "20 IF X = 0 THEN 50\n"
        "30 PRINT X;\n"
        "40 LET X = X - 1\n"
        "50 IF X > 0 THEN 30\n"
        "60 PRINT \"go!\"\n"
        "70 END\n"
    ), "54321go!\n");
}
