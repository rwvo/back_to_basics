#pragma once

#include <string>
#include <vector>
#include "ast.h"
#include "token.h"

namespace rocbas {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    Program parse();

private:
    std::vector<Token> tokens_;
    size_t pos_ = 0;

    // Token navigation
    const Token& peek() const;
    const Token& advance();
    bool at_end() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& expect(TokenType type, const std::string& message);

    // Statement parsing
    StmtPtr parse_line();
    StmtPtr parse_statement(int line_number);
    LetStmt parse_let();
    PrintStmt parse_print();
    InputStmt parse_input();
    GotoStmt parse_goto();
    GosubStmt parse_gosub();
    IfStmt parse_if(int line_number);
    ForStmt parse_for();
    NextStmt parse_next();
    DimStmt parse_dim();
    RemStmt parse_rem();

    // Expression parsing (precedence climbing)
    ExprPtr parse_expression();
    ExprPtr parse_or_expr();
    ExprPtr parse_and_expr();
    ExprPtr parse_not_expr();
    ExprPtr parse_comparison();
    ExprPtr parse_addition();
    ExprPtr parse_multiplication();
    ExprPtr parse_exponentiation();
    ExprPtr parse_unary();
    ExprPtr parse_primary();

    // Helpers
    bool is_builtin_function(const std::string& name) const;
};

} // namespace rocbas
