#include "parser.h"
#include <algorithm>
#include <set>
#include <stdexcept>

namespace rocbas {

static const std::set<std::string> builtin_functions = {
    "ABS", "INT", "SQR", "RND", "SIN", "COS", "TAN",
    "LEN", "LEFT$", "RIGHT$", "MID$", "CHR$", "ASC", "VAL", "STR$", "TAB",
};

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

const Token& Parser::peek() const {
    return tokens_[pos_];
}

const Token& Parser::advance() {
    const Token& tok = tokens_[pos_];
    if (!at_end()) pos_++;
    return tok;
}

bool Parser::at_end() const {
    return pos_ >= tokens_.size() || tokens_[pos_].type == TokenType::END_OF_FILE;
}

bool Parser::check(TokenType type) const {
    if (at_end()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::expect(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }
    throw std::runtime_error("Parse error: " + message +
                             " (got " + peek().lexeme + ")");
}

bool Parser::is_builtin_function(const std::string& name) const {
    return builtin_functions.count(name) > 0;
}

// --- Top-level parsing ---

Program Parser::parse() {
    Program program;
    std::set<int> seen_lines;

    while (!at_end()) {
        // Skip blank lines
        while (match(TokenType::NEWLINE)) {}
        if (at_end()) break;

        auto stmt = parse_line();
        if (stmt) {
            if (seen_lines.count(stmt->line_number)) {
                throw std::runtime_error("Duplicate line number: " +
                                         std::to_string(stmt->line_number));
            }
            seen_lines.insert(stmt->line_number);
            program.lines.push_back(std::move(stmt));
        }
    }

    // Sort by line number
    std::sort(program.lines.begin(), program.lines.end(),
              [](const StmtPtr& a, const StmtPtr& b) {
                  return a->line_number < b->line_number;
              });

    return program;
}

StmtPtr Parser::parse_line() {
    // Expect a line number
    if (!check(TokenType::INTEGER_LITERAL)) {
        throw std::runtime_error("Expected line number, got '" + peek().lexeme + "'");
    }
    int line_number = std::stoi(advance().lexeme);

    auto stmt = parse_statement(line_number);

    // Consume newline or EOF
    if (!at_end()) {
        match(TokenType::NEWLINE);
    }

    return stmt;
}

StmtPtr Parser::parse_statement(int line_number) {
    if (check(TokenType::KW_LET)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_let());
    }
    if (check(TokenType::KW_PRINT)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_print());
    }
    if (check(TokenType::KW_INPUT)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_input());
    }
    if (check(TokenType::KW_GOTO)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_goto());
    }
    if (check(TokenType::KW_GOSUB)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_gosub());
    }
    if (check(TokenType::KW_RETURN)) {
        advance();
        return std::make_unique<Statement>(line_number, ReturnStmt{});
    }
    if (check(TokenType::KW_IF)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_if(line_number));
    }
    if (check(TokenType::KW_FOR)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_for());
    }
    if (check(TokenType::KW_NEXT)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_next());
    }
    if (check(TokenType::KW_DIM)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_dim());
    }
    if (check(TokenType::KW_END)) {
        advance();
        return std::make_unique<Statement>(line_number, EndStmt{});
    }
    if (check(TokenType::KW_REM)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_rem());
    }

    // Optional LET: if we see an identifier followed by '=' or '('
    if (check(TokenType::IDENTIFIER) || check(TokenType::STRING_IDENTIFIER)) {
        return std::make_unique<Statement>(line_number, parse_let());
    }

    throw std::runtime_error("Unexpected token '" + peek().lexeme +
                             "' at line " + std::to_string(line_number));
}

// --- Statement implementations ---

LetStmt Parser::parse_let() {
    LetStmt stmt;

    // Get variable name
    if (check(TokenType::IDENTIFIER)) {
        stmt.var_name = advance().lexeme;
    } else if (check(TokenType::STRING_IDENTIFIER)) {
        stmt.var_name = advance().lexeme;
    } else {
        throw std::runtime_error("Expected variable name in LET");
    }

    // Check for array index: VAR(index)
    if (match(TokenType::LPAREN)) {
        stmt.indices.push_back(parse_expression());
        while (match(TokenType::COMMA)) {
            stmt.indices.push_back(parse_expression());
        }
        expect(TokenType::RPAREN, "Expected ')' after array index");
    }

    expect(TokenType::EQUALS, "Expected '=' in assignment");
    stmt.value = parse_expression();
    return stmt;
}

PrintStmt Parser::parse_print() {
    PrintStmt stmt;
    stmt.trailing_newline = true;

    // Handle empty PRINT (just prints newline)
    if (at_end() || check(TokenType::NEWLINE)) {
        return stmt;
    }

    while (true) {
        PrintStmt::Item item;
        item.expr = parse_expression();
        item.suppress_newline = false;

        if (match(TokenType::SEMICOLON)) {
            item.suppress_newline = true;
            stmt.items.push_back(std::move(item));

            // Check if semicolon is at end of statement
            if (at_end() || check(TokenType::NEWLINE)) {
                stmt.trailing_newline = false;
                break;
            }
            continue;
        }

        if (match(TokenType::COMMA)) {
            item.suppress_newline = true;  // comma acts like tab
            stmt.items.push_back(std::move(item));
            continue;
        }

        stmt.items.push_back(std::move(item));
        break;
    }

    return stmt;
}

InputStmt Parser::parse_input() {
    InputStmt stmt;
    stmt.prompt = "";

    // Check for optional prompt: INPUT "prompt"; VAR
    if (check(TokenType::STRING_LITERAL)) {
        stmt.prompt = advance().lexeme;
        expect(TokenType::SEMICOLON, "Expected ';' after INPUT prompt");
    }

    if (check(TokenType::IDENTIFIER)) {
        stmt.var_name = advance().lexeme;
    } else if (check(TokenType::STRING_IDENTIFIER)) {
        stmt.var_name = advance().lexeme;
    } else {
        throw std::runtime_error("Expected variable name in INPUT");
    }

    return stmt;
}

GotoStmt Parser::parse_goto() {
    auto& tok = expect(TokenType::INTEGER_LITERAL, "Expected line number after GOTO");
    return GotoStmt{std::stoi(tok.lexeme)};
}

GosubStmt Parser::parse_gosub() {
    auto& tok = expect(TokenType::INTEGER_LITERAL, "Expected line number after GOSUB");
    return GosubStmt{std::stoi(tok.lexeme)};
}

IfStmt Parser::parse_if(int line_number) {
    IfStmt stmt;
    stmt.condition = parse_expression();
    stmt.has_goto = false;
    stmt.goto_line = 0;
    stmt.then_stmt = nullptr;

    expect(TokenType::KW_THEN, "Expected THEN after IF condition");

    // THEN can be followed by a line number (implicit GOTO) or a statement
    if (check(TokenType::INTEGER_LITERAL)) {
        stmt.has_goto = true;
        stmt.goto_line = std::stoi(advance().lexeme);
    } else {
        stmt.has_goto = false;
        stmt.then_stmt = parse_statement(line_number);
    }

    return stmt;
}

ForStmt Parser::parse_for() {
    ForStmt stmt;

    auto& var = expect(TokenType::IDENTIFIER, "Expected variable name after FOR");
    stmt.var_name = var.lexeme;

    expect(TokenType::EQUALS, "Expected '=' in FOR");
    stmt.start = parse_expression();

    expect(TokenType::KW_TO, "Expected TO in FOR");
    stmt.end = parse_expression();

    stmt.step = nullptr;
    if (match(TokenType::KW_STEP)) {
        stmt.step = parse_expression();
    }

    return stmt;
}

NextStmt Parser::parse_next() {
    auto& var = expect(TokenType::IDENTIFIER, "Expected variable name after NEXT");
    return NextStmt{var.lexeme};
}

DimStmt Parser::parse_dim() {
    DimStmt stmt;

    do {
        DimStmt::ArrayDecl decl;
        if (check(TokenType::IDENTIFIER)) {
            decl.name = advance().lexeme;
        } else if (check(TokenType::STRING_IDENTIFIER)) {
            decl.name = advance().lexeme;
        } else {
            throw std::runtime_error("Expected array name in DIM");
        }

        expect(TokenType::LPAREN, "Expected '(' after array name in DIM");
        decl.dimensions.push_back(parse_expression());
        while (match(TokenType::COMMA)) {
            decl.dimensions.push_back(parse_expression());
        }
        expect(TokenType::RPAREN, "Expected ')' in DIM");

        stmt.arrays.push_back(std::move(decl));
    } while (match(TokenType::COMMA));

    return stmt;
}

RemStmt Parser::parse_rem() {
    // The lexer already captured the comment text in the REM token's lexeme.
    // But by the time we get here, the REM keyword token has been consumed.
    // The previous token (at pos_-1) is the REM token with the comment.
    return RemStmt{tokens_[pos_ - 1].lexeme};
}

// --- Expression parsing (precedence climbing) ---

ExprPtr Parser::parse_expression() {
    return parse_or_expr();
}

ExprPtr Parser::parse_or_expr() {
    auto left = parse_and_expr();
    while (match(TokenType::KW_OR)) {
        auto right = parse_and_expr();
        left = std::make_unique<Expression>(BinaryExpr{
            BinaryOp::OR, std::move(left), std::move(right)});
    }
    return left;
}

ExprPtr Parser::parse_and_expr() {
    auto left = parse_not_expr();
    while (match(TokenType::KW_AND)) {
        auto right = parse_not_expr();
        left = std::make_unique<Expression>(BinaryExpr{
            BinaryOp::AND, std::move(left), std::move(right)});
    }
    return left;
}

ExprPtr Parser::parse_not_expr() {
    if (match(TokenType::KW_NOT)) {
        auto operand = parse_not_expr();
        return std::make_unique<Expression>(UnaryExpr{
            UnaryOp::NOT, std::move(operand)});
    }
    return parse_comparison();
}

ExprPtr Parser::parse_comparison() {
    auto left = parse_addition();

    while (true) {
        BinaryOp op;
        if (check(TokenType::EQUALS))        { op = BinaryOp::EQ; }
        else if (check(TokenType::NOT_EQUAL)) { op = BinaryOp::NE; }
        else if (check(TokenType::LESS))      { op = BinaryOp::LT; }
        else if (check(TokenType::GREATER))   { op = BinaryOp::GT; }
        else if (check(TokenType::LESS_EQUAL))    { op = BinaryOp::LE; }
        else if (check(TokenType::GREATER_EQUAL)) { op = BinaryOp::GE; }
        else break;

        advance();
        auto right = parse_addition();
        left = std::make_unique<Expression>(BinaryExpr{
            op, std::move(left), std::move(right)});
    }
    return left;
}

ExprPtr Parser::parse_addition() {
    auto left = parse_multiplication();

    while (true) {
        BinaryOp op;
        if (check(TokenType::PLUS))       { op = BinaryOp::ADD; }
        else if (check(TokenType::MINUS))  { op = BinaryOp::SUB; }
        else break;

        advance();
        auto right = parse_multiplication();
        left = std::make_unique<Expression>(BinaryExpr{
            op, std::move(left), std::move(right)});
    }
    return left;
}

ExprPtr Parser::parse_multiplication() {
    auto left = parse_exponentiation();

    while (true) {
        BinaryOp op;
        if (check(TokenType::STAR))       { op = BinaryOp::MUL; }
        else if (check(TokenType::SLASH)) { op = BinaryOp::DIV; }
        else break;

        advance();
        auto right = parse_exponentiation();
        left = std::make_unique<Expression>(BinaryExpr{
            op, std::move(left), std::move(right)});
    }
    return left;
}

ExprPtr Parser::parse_exponentiation() {
    auto left = parse_unary();

    // Right-associative: 2^3^4 = 2^(3^4)
    if (match(TokenType::CARET)) {
        auto right = parse_exponentiation();
        return std::make_unique<Expression>(BinaryExpr{
            BinaryOp::POW, std::move(left), std::move(right)});
    }
    return left;
}

ExprPtr Parser::parse_unary() {
    if (match(TokenType::MINUS)) {
        auto operand = parse_unary();
        return std::make_unique<Expression>(UnaryExpr{
            UnaryOp::NEGATE, std::move(operand)});
    }
    return parse_primary();
}

ExprPtr Parser::parse_primary() {
    // Number literals
    if (check(TokenType::INTEGER_LITERAL) || check(TokenType::FLOAT_LITERAL)) {
        double val = std::stod(advance().lexeme);
        return std::make_unique<Expression>(NumberLiteral{val});
    }

    // String literals
    if (check(TokenType::STRING_LITERAL)) {
        std::string val = advance().lexeme;
        return std::make_unique<Expression>(StringLiteral{val});
    }

    // Parenthesized expression
    if (match(TokenType::LPAREN)) {
        auto expr = parse_expression();
        expect(TokenType::RPAREN, "Expected ')'");
        return expr;
    }

    // Identifier: could be variable, array access, or function call
    if (check(TokenType::IDENTIFIER) || check(TokenType::STRING_IDENTIFIER)) {
        std::string name = advance().lexeme;

        // Function call or array access: NAME(args)
        if (match(TokenType::LPAREN)) {
            std::vector<ExprPtr> args;
            if (!check(TokenType::RPAREN)) {
                args.push_back(parse_expression());
                while (match(TokenType::COMMA)) {
                    args.push_back(parse_expression());
                }
            }
            expect(TokenType::RPAREN, "Expected ')' after arguments");

            if (is_builtin_function(name)) {
                return std::make_unique<Expression>(FunctionCall{name, std::move(args)});
            }
            // Otherwise it's an array access
            return std::make_unique<Expression>(ArrayAccess{name, std::move(args)});
        }

        // Simple variable
        return std::make_unique<Expression>(Variable{name});
    }

    throw std::runtime_error("Unexpected token in expression: '" + peek().lexeme + "'");
}

} // namespace rocbas
