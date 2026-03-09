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

// --- Arrays ---

TEST(Interpreter, ArrayDimAndAccess) {
    EXPECT_EQ(run(
        "10 DIM A(5)\n"
        "20 LET A(1) = 10\n"
        "30 LET A(2) = 20\n"
        "40 PRINT A(1) + A(2)\n"
        "50 END\n"
    ), "30\n");
}

TEST(Interpreter, ArrayInLoop) {
    EXPECT_EQ(run(
        "10 DIM A(5)\n"
        "20 FOR I = 1 TO 5\n"
        "30 LET A(I) = I * I\n"
        "40 NEXT I\n"
        "50 FOR I = 1 TO 5\n"
        "60 PRINT A(I);\n"
        "70 NEXT I\n"
        "80 END\n"
    ), "1491625");
}

TEST(Interpreter, ArrayBoundsError) {
    EXPECT_THROW(run(
        "10 DIM A(5)\n"
        "20 LET A(6) = 1\n"
        "30 END\n"
    ), std::runtime_error);
}

TEST(Interpreter, MultiDimArray) {
    EXPECT_EQ(run(
        "10 DIM A(3, 3)\n"
        "20 LET A(2, 3) = 42\n"
        "30 PRINT A(2, 3)\n"
        "40 END\n"
    ), "42\n");
}

TEST(Interpreter, BubbleSort) {
    EXPECT_EQ(run(
        "10 DIM A(5)\n"
        "20 A(1) = 5\n"
        "30 A(2) = 3\n"
        "40 A(3) = 1\n"
        "50 A(4) = 4\n"
        "60 A(5) = 2\n"
        "70 REM Bubble sort\n"
        "80 FOR I = 1 TO 4\n"
        "90 FOR J = 1 TO 5 - I\n"
        "100 IF A(J) <= A(J + 1) THEN 130\n"
        "110 LET T = A(J)\n"
        "120 A(J) = A(J + 1)\n"
        "125 A(J + 1) = T\n"
        "130 NEXT J\n"
        "140 NEXT I\n"
        "150 FOR I = 1 TO 5\n"
        "160 PRINT A(I);\n"
        "170 NEXT I\n"
        "180 END\n"
    ), "12345");
}

// --- Countdown with GOTO loop ---

// --- Built-in functions ---

TEST(Interpreter, FuncAbs) {
    EXPECT_EQ(run("10 PRINT ABS(-42)\n20 END\n"), "42\n");
}

TEST(Interpreter, FuncInt) {
    EXPECT_EQ(run("10 PRINT INT(3.7)\n20 END\n"), "3\n");
    EXPECT_EQ(run("10 PRINT INT(-3.2)\n20 END\n"), "-4\n");
}

TEST(Interpreter, FuncSqr) {
    EXPECT_EQ(run("10 PRINT SQR(16)\n20 END\n"), "4\n");
}

TEST(Interpreter, FuncSinCos) {
    EXPECT_EQ(run("10 PRINT SIN(0)\n20 END\n"), "0\n");
    EXPECT_EQ(run("10 PRINT COS(0)\n20 END\n"), "1\n");
}

TEST(Interpreter, FuncLen) {
    EXPECT_EQ(run("10 PRINT LEN(\"hello\")\n20 END\n"), "5\n");
}

TEST(Interpreter, FuncLeft) {
    EXPECT_EQ(run("10 PRINT LEFT$(\"hello\", 3)\n20 END\n"), "hel\n");
}

TEST(Interpreter, FuncRight) {
    EXPECT_EQ(run("10 PRINT RIGHT$(\"hello\", 3)\n20 END\n"), "llo\n");
}

TEST(Interpreter, FuncMid) {
    EXPECT_EQ(run("10 PRINT MID$(\"hello\", 2, 3)\n20 END\n"), "ell\n");
}

TEST(Interpreter, FuncChrAsc) {
    EXPECT_EQ(run("10 PRINT CHR$(65)\n20 END\n"), "A\n");
    EXPECT_EQ(run("10 PRINT ASC(\"A\")\n20 END\n"), "65\n");
}

TEST(Interpreter, FuncVal) {
    EXPECT_EQ(run("10 PRINT VAL(\"42\")\n20 END\n"), "42\n");
}

TEST(Interpreter, FuncStr) {
    EXPECT_EQ(run("10 PRINT STR$(42)\n20 END\n"), "42\n");
}

// --- Error handling ---

TEST(Interpreter, UndefinedLineGoto) {
    EXPECT_THROW(run("10 GOTO 999\n"), std::runtime_error);
}

TEST(Interpreter, DivisionByZero) {
    EXPECT_THROW(run("10 PRINT 1 / 0\n20 END\n"), std::runtime_error);
}

TEST(Interpreter, TypeMismatch) {
    EXPECT_THROW(run("10 PRINT \"hello\" + 5\n20 END\n"), std::runtime_error);
}

// --- INPUT (stream injection) ---

TEST(Interpreter, InputNumeric) {
    std::ostringstream out;
    std::istringstream in("42\n");
    rocbas::Interpreter interp(out, in);
    interp.run("10 INPUT X\n20 PRINT X\n30 END\n");
    EXPECT_EQ(out.str(), "? 42\n");
}

TEST(Interpreter, InputWithPrompt) {
    std::ostringstream out;
    std::istringstream in("hello\n");
    rocbas::Interpreter interp(out, in);
    interp.run("10 INPUT \"Name\"; N$\n20 PRINT N$\n30 END\n");
    EXPECT_EQ(out.str(), "Name? hello\n");
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

// --- OPTION BASE ---

TEST(Interpreter, OptionBase0) {
    EXPECT_EQ(run(
        "5 OPTION BASE 0\n"
        "10 DIM A(5)\n"
        "20 LET A(0) = 10\n"
        "30 LET A(4) = 40\n"
        "40 PRINT A(0); A(4)\n"
        "50 END\n"
    ), "1040\n");
}

TEST(Interpreter, OptionBase0BoundsCheck) {
    // A(5) should be out of bounds with OPTION BASE 0 and DIM A(5) — valid indices are 0..4
    EXPECT_THROW(run(
        "5 OPTION BASE 0\n"
        "10 DIM A(5)\n"
        "20 LET A(5) = 1\n"
        "30 END\n"
    ), std::runtime_error);
}

TEST(Interpreter, OptionBase1Default) {
    // Default base is 1 — A(0) should throw
    EXPECT_THROW(run(
        "10 DIM A(5)\n"
        "20 LET A(0) = 1\n"
        "30 END\n"
    ), std::runtime_error);
}

TEST(Interpreter, OptionBase0InLoop) {
    EXPECT_EQ(run(
        "5 OPTION BASE 0\n"
        "10 DIM A(5)\n"
        "20 FOR I = 0 TO 4\n"
        "30 LET A(I) = I * 10\n"
        "40 NEXT I\n"
        "50 PRINT A(0); A(2); A(4)\n"
        "60 END\n"
    ), "02040\n");
}

TEST(Interpreter, OptionBaseParseError) {
    // OPTION BASE 2 should fail — only 0, 0.5, or 1 allowed
    EXPECT_THROW(run(
        "10 OPTION BASE 2\n"
        "20 END\n"
    ), std::runtime_error);
}

// --- OPTION BASE 0.5 ---

TEST(Interpreter, OptionBaseHalf) {
    EXPECT_EQ(run(
        "5 OPTION BASE 0.5\n"
        "10 DIM A(5)\n"
        "20 LET A(0.5) = 10\n"
        "30 LET A(1.5) = 20\n"
        "40 LET A(4.5) = 50\n"
        "50 PRINT A(0.5); A(1.5); A(4.5)\n"
        "60 END\n"
    ), "102050\n");
}

TEST(Interpreter, OptionBaseHalfBoundsCheck) {
    // A(5.5) should be out of bounds with OPTION BASE 0.5 and DIM A(5) — valid indices are 0.5..4.5
    EXPECT_THROW(run(
        "5 OPTION BASE 0.5\n"
        "10 DIM A(5)\n"
        "20 LET A(5.5) = 1\n"
        "30 END\n"
    ), std::runtime_error);
}

TEST(Interpreter, OptionBaseHalfInLoop) {
    EXPECT_EQ(run(
        "5 OPTION BASE 0.5\n"
        "10 DIM A(5)\n"
        "20 FOR I = 0 TO 4\n"
        "30 LET A(I + 0.5) = (I + 1) * 10\n"
        "40 NEXT I\n"
        "50 PRINT A(0.5); A(2.5); A(4.5)\n"
        "60 END\n"
    ), "103050\n");
}

TEST(Interpreter, OptionBaseParseErrorHalf) {
    // OPTION BASE 0.3 should fail — only 0, 0.5, or 1 allowed
    EXPECT_THROW(run(
        "10 OPTION BASE 0.3\n"
        "20 END\n"
    ), std::runtime_error);
}
