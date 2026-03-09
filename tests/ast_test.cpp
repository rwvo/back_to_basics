#include <gtest/gtest.h>
#include "ast.h"

using namespace rocbas;

TEST(AST, NumberLiteral) {
    auto expr = make_expr(42.0);
    ASSERT_TRUE(std::holds_alternative<NumberLiteral>(expr->expr));
    EXPECT_DOUBLE_EQ(std::get<NumberLiteral>(expr->expr).value, 42.0);
}

TEST(AST, StringLiteral) {
    auto expr = make_expr(std::string("hello"));
    ASSERT_TRUE(std::holds_alternative<StringLiteral>(expr->expr));
    EXPECT_EQ(std::get<StringLiteral>(expr->expr).value, "hello");
}

TEST(AST, Variable) {
    auto expr = make_var("X");
    ASSERT_TRUE(std::holds_alternative<Variable>(expr->expr));
    EXPECT_EQ(std::get<Variable>(expr->expr).name, "X");
}

TEST(AST, BinaryExpression) {
    // Build: 2 + 3
    auto expr = std::make_unique<Expression>(BinaryExpr{
        BinaryOp::ADD,
        make_expr(2.0),
        make_expr(3.0)
    });
    ASSERT_TRUE(std::holds_alternative<BinaryExpr>(expr->expr));
    auto& bin = std::get<BinaryExpr>(expr->expr);
    EXPECT_EQ(bin.op, BinaryOp::ADD);
    EXPECT_TRUE(std::holds_alternative<NumberLiteral>(bin.left->expr));
    EXPECT_TRUE(std::holds_alternative<NumberLiteral>(bin.right->expr));
}

TEST(AST, UnaryExpression) {
    // Build: -5
    auto expr = std::make_unique<Expression>(UnaryExpr{
        UnaryOp::NEGATE,
        make_expr(5.0)
    });
    ASSERT_TRUE(std::holds_alternative<UnaryExpr>(expr->expr));
}

TEST(AST, LetStatement) {
    // Build: LET X = 42
    auto stmt = std::make_unique<Statement>(10, LetStmt{
        "X", {}, make_expr(42.0)
    });
    EXPECT_EQ(stmt->line_number, 10);
    ASSERT_TRUE(std::holds_alternative<LetStmt>(stmt->stmt));
    auto& let = std::get<LetStmt>(stmt->stmt);
    EXPECT_EQ(let.var_name, "X");
    EXPECT_TRUE(let.indices.empty());
}

TEST(AST, PrintStatement) {
    // Build: PRINT "HELLO"
    PrintStmt print_stmt;
    print_stmt.trailing_newline = true;
    PrintStmt::Item item;
    item.expr = make_expr(std::string("HELLO"));
    item.suppress_newline = false;
    print_stmt.items.push_back(std::move(item));

    auto stmt = std::make_unique<Statement>(20, std::move(print_stmt));
    EXPECT_EQ(stmt->line_number, 20);
    ASSERT_TRUE(std::holds_alternative<PrintStmt>(stmt->stmt));
}

TEST(AST, ForNextStatements) {
    // Build: FOR I = 1 TO 10 STEP 2
    auto for_stmt = std::make_unique<Statement>(30, ForStmt{
        "I", make_expr(1.0), make_expr(10.0), make_expr(2.0)
    });
    EXPECT_EQ(for_stmt->line_number, 30);
    ASSERT_TRUE(std::holds_alternative<ForStmt>(for_stmt->stmt));
    auto& f = std::get<ForStmt>(for_stmt->stmt);
    EXPECT_EQ(f.var_name, "I");
    EXPECT_NE(f.step, nullptr);

    // Build: NEXT I
    auto next_stmt = std::make_unique<Statement>(40, NextStmt{"I"});
    ASSERT_TRUE(std::holds_alternative<NextStmt>(next_stmt->stmt));
}

TEST(AST, DimStatement) {
    // Build: DIM A(10, 20)
    DimStmt dim;
    DimStmt::ArrayDecl decl;
    decl.name = "A";
    decl.dimensions.push_back(make_expr(10.0));
    decl.dimensions.push_back(make_expr(20.0));
    dim.arrays.push_back(std::move(decl));

    auto stmt = std::make_unique<Statement>(50, std::move(dim));
    ASSERT_TRUE(std::holds_alternative<DimStmt>(stmt->stmt));
    auto& d = std::get<DimStmt>(stmt->stmt);
    EXPECT_EQ(d.arrays.size(), 1u);
    EXPECT_EQ(d.arrays[0].name, "A");
    EXPECT_EQ(d.arrays[0].dimensions.size(), 2u);
}

TEST(AST, Program) {
    Program prog;
    prog.lines.push_back(std::make_unique<Statement>(10, RemStmt{"Test"}));
    prog.lines.push_back(std::make_unique<Statement>(20, EndStmt{}));
    EXPECT_EQ(prog.lines.size(), 2u);
    EXPECT_EQ(prog.lines[0]->line_number, 10);
    EXPECT_EQ(prog.lines[1]->line_number, 20);
}
