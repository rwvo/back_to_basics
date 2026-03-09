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
        // Check for END KERNEL
        if (check(TokenType::KW_KERNEL)) {
            // END KERNEL is handled by parse_gpu_kernel; shouldn't appear standalone
            throw std::runtime_error("END KERNEL without matching GPU KERNEL at line " +
                                     std::to_string(line_number));
        }
        return std::make_unique<Statement>(line_number, EndStmt{});
    }
    if (check(TokenType::KW_GPU)) {
        advance();
        return parse_gpu_statement(line_number);
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

    // GPU intrinsics: THREAD_IDX(d), BLOCK_IDX(d), BLOCK_DIM(d), GRID_DIM(d)
    if (check(TokenType::KW_THREAD_IDX) || check(TokenType::KW_BLOCK_IDX) ||
        check(TokenType::KW_BLOCK_DIM) || check(TokenType::KW_GRID_DIM)) {
        GpuIntrinsicKind kind;
        switch (advance().type) {
            case TokenType::KW_THREAD_IDX: kind = GpuIntrinsicKind::THREAD_IDX; break;
            case TokenType::KW_BLOCK_IDX:  kind = GpuIntrinsicKind::BLOCK_IDX; break;
            case TokenType::KW_BLOCK_DIM:  kind = GpuIntrinsicKind::BLOCK_DIM; break;
            case TokenType::KW_GRID_DIM:   kind = GpuIntrinsicKind::GRID_DIM; break;
            default: throw std::runtime_error("Unexpected GPU intrinsic");
        }
        expect(TokenType::LPAREN, "Expected '(' after GPU intrinsic");
        auto dim = parse_expression();
        expect(TokenType::RPAREN, "Expected ')' after GPU intrinsic dimension");
        return std::make_unique<Expression>(GpuIntrinsic{kind, std::move(dim)});
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

// --- GPU statement parsing ---

StmtPtr Parser::parse_gpu_statement(int line_number) {
    // GPU has been consumed; next token determines the GPU statement type
    if (check(TokenType::KW_DIM)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_gpu_dim());
    }
    if (check(TokenType::KW_COPY)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_gpu_copy());
    }
    if (check(TokenType::KW_FREE)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_gpu_free());
    }
    if (check(TokenType::KW_KERNEL)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_gpu_kernel(line_number));
    }
    if (check(TokenType::KW_GOSUB)) {
        advance();
        return std::make_unique<Statement>(line_number, parse_gpu_gosub());
    }
    throw std::runtime_error("Expected DIM, COPY, FREE, KERNEL, or GOSUB after GPU at line " +
                             std::to_string(line_number));
}

GpuDimStmt Parser::parse_gpu_dim() {
    GpuDimStmt stmt;
    do {
        GpuDimStmt::ArrayDecl decl;
        decl.name = expect(TokenType::IDENTIFIER, "Expected array name in GPU DIM").lexeme;
        expect(TokenType::LPAREN, "Expected '(' after array name in GPU DIM");
        decl.dimensions.push_back(parse_expression());
        while (match(TokenType::COMMA)) {
            decl.dimensions.push_back(parse_expression());
        }
        expect(TokenType::RPAREN, "Expected ')' in GPU DIM");
        stmt.arrays.push_back(std::move(decl));
    } while (match(TokenType::COMMA));
    return stmt;
}

GpuCopyStmt Parser::parse_gpu_copy() {
    // GPU COPY src TO dst
    std::string src;
    if (check(TokenType::IDENTIFIER)) {
        src = advance().lexeme;
    } else {
        throw std::runtime_error("Expected variable name after GPU COPY");
    }
    expect(TokenType::KW_TO, "Expected TO in GPU COPY");
    std::string dst;
    if (check(TokenType::IDENTIFIER)) {
        dst = advance().lexeme;
    } else {
        throw std::runtime_error("Expected variable name after TO in GPU COPY");
    }
    return GpuCopyStmt{src, dst};
}

GpuFreeStmt Parser::parse_gpu_free() {
    std::string name = expect(TokenType::IDENTIFIER, "Expected array name in GPU FREE").lexeme;
    return GpuFreeStmt{name};
}

GpuKernelStmt Parser::parse_gpu_kernel(int line_number) {
    // GPU KERNEL name(param1, param2, ...)
    GpuKernelStmt stmt;
    stmt.name = expect(TokenType::IDENTIFIER, "Expected kernel name after GPU KERNEL").lexeme;

    expect(TokenType::LPAREN, "Expected '(' after kernel name");
    if (!check(TokenType::RPAREN)) {
        stmt.params.push_back(
            expect(TokenType::IDENTIFIER, "Expected parameter name").lexeme);
        while (match(TokenType::COMMA)) {
            stmt.params.push_back(
                expect(TokenType::IDENTIFIER, "Expected parameter name").lexeme);
        }
    }
    expect(TokenType::RPAREN, "Expected ')' after kernel parameters");

    // Consume newline after kernel declaration line
    match(TokenType::NEWLINE);

    // Parse kernel body lines until END KERNEL
    while (!at_end()) {
        // Skip blank lines
        while (match(TokenType::NEWLINE)) {}
        if (at_end()) break;

        // Check for END KERNEL
        if (check(TokenType::INTEGER_LITERAL)) {
            // Peek ahead: is this "NNN END KERNEL"?
            size_t saved_pos = pos_;
            int body_line = std::stoi(advance().lexeme);

            if (check(TokenType::KW_END)) {
                advance();
                if (check(TokenType::KW_KERNEL)) {
                    advance();
                    match(TokenType::NEWLINE);
                    // END KERNEL found — the kernel definition ends here.
                    // We store the END KERNEL line number as the statement's line number.
                    return stmt;
                }
                // Was just END (not END KERNEL) — that's an error inside a kernel
                throw std::runtime_error("END without KERNEL inside GPU KERNEL at line " +
                                         std::to_string(body_line));
            }

            // Not END — parse as a kernel body statement
            auto body_stmt = parse_statement(body_line);
            match(TokenType::NEWLINE);
            stmt.body.push_back(std::move(body_stmt));
        } else {
            throw std::runtime_error("Expected line number in GPU KERNEL body");
        }
    }
    throw std::runtime_error("GPU KERNEL without END KERNEL");
}

GpuGosubStmt Parser::parse_gpu_gosub() {
    // GPU GOSUB name(args) WITH grid BLOCKS OF block
    GpuGosubStmt stmt;
    stmt.kernel_name = expect(TokenType::IDENTIFIER, "Expected kernel name after GPU GOSUB").lexeme;

    expect(TokenType::LPAREN, "Expected '(' after kernel name");
    if (!check(TokenType::RPAREN)) {
        stmt.args.push_back(parse_expression());
        while (match(TokenType::COMMA)) {
            stmt.args.push_back(parse_expression());
        }
    }
    expect(TokenType::RPAREN, "Expected ')' after kernel arguments");

    expect(TokenType::KW_WITH, "Expected WITH after kernel arguments");

    // Grid dimensions: either a single number or (nx, ny, nz)
    if (match(TokenType::LPAREN)) {
        stmt.grid_dims.push_back(parse_expression());
        while (match(TokenType::COMMA)) {
            stmt.grid_dims.push_back(parse_expression());
        }
        expect(TokenType::RPAREN, "Expected ')' after grid dimensions");
    } else {
        stmt.grid_dims.push_back(parse_expression());
    }

    expect(TokenType::KW_BLOCKS, "Expected BLOCKS after grid size");
    expect(TokenType::KW_OF, "Expected OF after BLOCKS");

    // Block dimensions: either a single number or (bx, by, bz)
    if (match(TokenType::LPAREN)) {
        stmt.block_dims.push_back(parse_expression());
        while (match(TokenType::COMMA)) {
            stmt.block_dims.push_back(parse_expression());
        }
        expect(TokenType::RPAREN, "Expected ')' after block dimensions");
    } else {
        stmt.block_dims.push_back(parse_expression());
    }

    return stmt;
}

} // namespace rocbas
