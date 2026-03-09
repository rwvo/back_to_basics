#include <gtest/gtest.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"

using namespace rocbas;

// Helper: parse source string into a Program
static Program parse(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    return parser.parse();
}

// Helper: get a specific statement variant
template <typename T>
const T& get_stmt(const Program& prog, size_t index) {
    return std::get<T>(prog.lines.at(index)->stmt);
}

// Helper: get expression variant from ExprPtr
template <typename T>
const T& get_expr(const ExprPtr& expr) {
    return std::get<T>(expr->expr);
}

// --- Expression parsing ---

TEST(Parser, NumberExpression) {
    auto prog = parse("10 LET X = 42");
    ASSERT_EQ(prog.lines.size(), 1u);
    auto& let = get_stmt<LetStmt>(prog, 0);
    ASSERT_TRUE(std::holds_alternative<NumberLiteral>(let.value->expr));
    EXPECT_DOUBLE_EQ(get_expr<NumberLiteral>(let.value).value, 42.0);
}

TEST(Parser, FloatExpression) {
    auto prog = parse("10 LET X = 3.14");
    auto& let = get_stmt<LetStmt>(prog, 0);
    EXPECT_DOUBLE_EQ(get_expr<NumberLiteral>(let.value).value, 3.14);
}

TEST(Parser, StringExpression) {
    auto prog = parse("10 LET A$ = \"hello\"");
    auto& let = get_stmt<LetStmt>(prog, 0);
    EXPECT_EQ(let.var_name, "A$");
    EXPECT_EQ(get_expr<StringLiteral>(let.value).value, "hello");
}

TEST(Parser, ArithmeticPrecedence) {
    // 2 + 3 * 4 should parse as 2 + (3 * 4)
    auto prog = parse("10 LET X = 2 + 3 * 4");
    auto& let = get_stmt<LetStmt>(prog, 0);
    auto& add = get_expr<BinaryExpr>(let.value);
    EXPECT_EQ(add.op, BinaryOp::ADD);
    EXPECT_DOUBLE_EQ(get_expr<NumberLiteral>(add.left).value, 2.0);
    auto& mul = get_expr<BinaryExpr>(add.right);
    EXPECT_EQ(mul.op, BinaryOp::MUL);
    EXPECT_DOUBLE_EQ(get_expr<NumberLiteral>(mul.left).value, 3.0);
    EXPECT_DOUBLE_EQ(get_expr<NumberLiteral>(mul.right).value, 4.0);
}

TEST(Parser, Parentheses) {
    // (2 + 3) * 4 should parse as (2 + 3) * 4
    auto prog = parse("10 LET X = (2 + 3) * 4");
    auto& let = get_stmt<LetStmt>(prog, 0);
    auto& mul = get_expr<BinaryExpr>(let.value);
    EXPECT_EQ(mul.op, BinaryOp::MUL);
    auto& add = get_expr<BinaryExpr>(mul.left);
    EXPECT_EQ(add.op, BinaryOp::ADD);
}

TEST(Parser, UnaryNegation) {
    auto prog = parse("10 LET X = -5");
    auto& let = get_stmt<LetStmt>(prog, 0);
    auto& unary = get_expr<UnaryExpr>(let.value);
    EXPECT_EQ(unary.op, UnaryOp::NEGATE);
    EXPECT_DOUBLE_EQ(get_expr<NumberLiteral>(unary.operand).value, 5.0);
}

TEST(Parser, ComparisonExpression) {
    auto prog = parse("10 IF X > 5 THEN END");
    auto& if_stmt = get_stmt<IfStmt>(prog, 0);
    auto& cmp = get_expr<BinaryExpr>(if_stmt.condition);
    EXPECT_EQ(cmp.op, BinaryOp::GT);
}

TEST(Parser, LogicalExpression) {
    auto prog = parse("10 IF X > 5 AND Y < 10 THEN END");
    auto& if_stmt = get_stmt<IfStmt>(prog, 0);
    auto& logic = get_expr<BinaryExpr>(if_stmt.condition);
    EXPECT_EQ(logic.op, BinaryOp::AND);
}

TEST(Parser, Exponentiation) {
    auto prog = parse("10 LET X = 2 ^ 3");
    auto& let = get_stmt<LetStmt>(prog, 0);
    auto& pow = get_expr<BinaryExpr>(let.value);
    EXPECT_EQ(pow.op, BinaryOp::POW);
}

TEST(Parser, FunctionCall) {
    auto prog = parse("10 LET X = ABS(-5)");
    auto& let = get_stmt<LetStmt>(prog, 0);
    auto& func = get_expr<FunctionCall>(let.value);
    EXPECT_EQ(func.name, "ABS");
    ASSERT_EQ(func.args.size(), 1u);
}

TEST(Parser, VariableExpression) {
    auto prog = parse("10 LET X = Y");
    auto& let = get_stmt<LetStmt>(prog, 0);
    auto& var = get_expr<Variable>(let.value);
    EXPECT_EQ(var.name, "Y");
}

// --- Statement parsing ---

TEST(Parser, LetStatement) {
    auto prog = parse("10 LET X = 42");
    ASSERT_EQ(prog.lines.size(), 1u);
    EXPECT_EQ(prog.lines[0]->line_number, 10);
    auto& let = get_stmt<LetStmt>(prog, 0);
    EXPECT_EQ(let.var_name, "X");
}

TEST(Parser, LetOptional) {
    // LET is optional
    auto prog = parse("10 X = 42");
    ASSERT_EQ(prog.lines.size(), 1u);
    auto& let = get_stmt<LetStmt>(prog, 0);
    EXPECT_EQ(let.var_name, "X");
    EXPECT_DOUBLE_EQ(get_expr<NumberLiteral>(let.value).value, 42.0);
}

TEST(Parser, PrintString) {
    auto prog = parse("10 PRINT \"HELLO\"");
    auto& print = get_stmt<PrintStmt>(prog, 0);
    ASSERT_EQ(print.items.size(), 1u);
    EXPECT_EQ(get_expr<StringLiteral>(print.items[0].expr).value, "HELLO");
    EXPECT_TRUE(print.trailing_newline);
}

TEST(Parser, PrintMultipleItems) {
    auto prog = parse("10 PRINT \"X=\"; X");
    auto& print = get_stmt<PrintStmt>(prog, 0);
    ASSERT_EQ(print.items.size(), 2u);
    EXPECT_TRUE(print.items[0].suppress_newline);
}

TEST(Parser, GotoStatement) {
    auto prog = parse("10 GOTO 100");
    auto& gt = get_stmt<GotoStmt>(prog, 0);
    EXPECT_EQ(gt.target_line, 100);
}

TEST(Parser, GosubReturn) {
    auto prog = parse("10 GOSUB 100\n20 RETURN");
    ASSERT_EQ(prog.lines.size(), 2u);
    auto& gosub = get_stmt<GosubStmt>(prog, 0);
    EXPECT_EQ(gosub.target_line, 100);
    ASSERT_TRUE(std::holds_alternative<ReturnStmt>(prog.lines[1]->stmt));
}

TEST(Parser, IfThenStatement) {
    auto prog = parse("10 IF X > 5 THEN PRINT \"BIG\"");
    auto& if_stmt = get_stmt<IfStmt>(prog, 0);
    EXPECT_FALSE(if_stmt.has_goto);
    ASSERT_NE(if_stmt.then_stmt, nullptr);
    ASSERT_TRUE(std::holds_alternative<PrintStmt>(if_stmt.then_stmt->stmt));
}

TEST(Parser, IfThenGoto) {
    auto prog = parse("10 IF X > 5 THEN 100");
    auto& if_stmt = get_stmt<IfStmt>(prog, 0);
    EXPECT_TRUE(if_stmt.has_goto);
    EXPECT_EQ(if_stmt.goto_line, 100);
}

TEST(Parser, ForNextLoop) {
    auto prog = parse("10 FOR I = 1 TO 10\n20 NEXT I");
    ASSERT_EQ(prog.lines.size(), 2u);
    auto& for_stmt = get_stmt<ForStmt>(prog, 0);
    EXPECT_EQ(for_stmt.var_name, "I");
    EXPECT_EQ(for_stmt.step, nullptr);  // default step
    auto& next = get_stmt<NextStmt>(prog, 1);
    EXPECT_EQ(next.var_name, "I");
}

TEST(Parser, ForNextWithStep) {
    auto prog = parse("10 FOR I = 0 TO 100 STEP 5");
    auto& for_stmt = get_stmt<ForStmt>(prog, 0);
    EXPECT_EQ(for_stmt.var_name, "I");
    ASSERT_NE(for_stmt.step, nullptr);
    EXPECT_DOUBLE_EQ(get_expr<NumberLiteral>(for_stmt.step).value, 5.0);
}

TEST(Parser, DimStatement) {
    auto prog = parse("10 DIM A(10)");
    auto& dim = get_stmt<DimStmt>(prog, 0);
    ASSERT_EQ(dim.arrays.size(), 1u);
    EXPECT_EQ(dim.arrays[0].name, "A");
    ASSERT_EQ(dim.arrays[0].dimensions.size(), 1u);
}

TEST(Parser, DimMultipleArrays) {
    auto prog = parse("10 DIM A(10), B(20, 30)");
    auto& dim = get_stmt<DimStmt>(prog, 0);
    ASSERT_EQ(dim.arrays.size(), 2u);
    EXPECT_EQ(dim.arrays[0].name, "A");
    EXPECT_EQ(dim.arrays[1].name, "B");
    EXPECT_EQ(dim.arrays[1].dimensions.size(), 2u);
}

TEST(Parser, EndStatement) {
    auto prog = parse("10 END");
    ASSERT_TRUE(std::holds_alternative<EndStmt>(prog.lines[0]->stmt));
}

TEST(Parser, RemStatement) {
    auto prog = parse("10 REM this is a comment");
    auto& rem = get_stmt<RemStmt>(prog, 0);
    EXPECT_EQ(rem.comment, "this is a comment");
}

TEST(Parser, InputStatement) {
    auto prog = parse("10 INPUT X");
    auto& input = get_stmt<InputStmt>(prog, 0);
    EXPECT_EQ(input.var_name, "X");
    EXPECT_EQ(input.prompt, "");
}

TEST(Parser, InputWithPrompt) {
    auto prog = parse("10 INPUT \"Enter number\"; X");
    auto& input = get_stmt<InputStmt>(prog, 0);
    EXPECT_EQ(input.var_name, "X");
    EXPECT_EQ(input.prompt, "Enter number");
}

// --- Multi-line programs ---

TEST(Parser, MultiLineProgram) {
    auto prog = parse(
        "10 REM Hello World\n"
        "20 PRINT \"Hello, World!\"\n"
        "30 END\n"
    );
    ASSERT_EQ(prog.lines.size(), 3u);
    EXPECT_EQ(prog.lines[0]->line_number, 10);
    EXPECT_EQ(prog.lines[1]->line_number, 20);
    EXPECT_EQ(prog.lines[2]->line_number, 30);
    ASSERT_TRUE(std::holds_alternative<RemStmt>(prog.lines[0]->stmt));
    ASSERT_TRUE(std::holds_alternative<PrintStmt>(prog.lines[1]->stmt));
    ASSERT_TRUE(std::holds_alternative<EndStmt>(prog.lines[2]->stmt));
}

TEST(Parser, ArrayAssignment) {
    auto prog = parse("10 LET A(5) = 42");
    auto& let = get_stmt<LetStmt>(prog, 0);
    EXPECT_EQ(let.var_name, "A");
    ASSERT_EQ(let.indices.size(), 1u);
    EXPECT_DOUBLE_EQ(get_expr<NumberLiteral>(let.indices[0]).value, 5.0);
}

TEST(Parser, ArrayAccessInExpression) {
    auto prog = parse("10 LET X = A(5)");
    auto& let = get_stmt<LetStmt>(prog, 0);
    auto& access = get_expr<ArrayAccess>(let.value);
    EXPECT_EQ(access.name, "A");
    ASSERT_EQ(access.indices.size(), 1u);
}

// --- Error cases ---

TEST(Parser, MissingLineNumber) {
    EXPECT_THROW(parse("PRINT \"hello\""), std::runtime_error);
}

TEST(Parser, DuplicateLineNumber) {
    EXPECT_THROW(parse("10 PRINT \"A\"\n10 PRINT \"B\""), std::runtime_error);
}
