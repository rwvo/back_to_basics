#include <gtest/gtest.h>
#include "lexer.h"

using namespace rocbas;

// Helper to tokenize and return token vector (excluding final EOF)
static std::vector<Token> toks(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    // Remove trailing EOF for easier assertions
    if (!tokens.empty() && tokens.back().type == TokenType::END_OF_FILE) {
        tokens.pop_back();
    }
    return tokens;
}

// --- Basic tokenization ---

TEST(Lexer, EmptyInput) {
    Lexer lexer("");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::END_OF_FILE);
}

TEST(Lexer, SimpleLineNumber) {
    auto tokens = toks("10");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[0].lexeme, "10");
}

TEST(Lexer, PrintStatement) {
    auto tokens = toks("10 PRINT \"HELLO\"");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[0].lexeme, "10");
    EXPECT_EQ(tokens[1].type, TokenType::KW_PRINT);
    EXPECT_EQ(tokens[2].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[2].lexeme, "HELLO");
}

TEST(Lexer, LetAssignment) {
    auto tokens = toks("20 LET X = 42");
    ASSERT_EQ(tokens.size(), 5u);
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[1].type, TokenType::KW_LET);
    EXPECT_EQ(tokens[2].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].lexeme, "X");
    EXPECT_EQ(tokens[3].type, TokenType::EQUALS);
    EXPECT_EQ(tokens[4].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[4].lexeme, "42");
}

// --- Keywords ---

TEST(Lexer, AllBasicKeywords) {
    auto tokens = toks("LET PRINT INPUT GOTO GOSUB RETURN IF THEN FOR TO STEP NEXT DIM END REM");
    ASSERT_EQ(tokens.size(), 15u);
    EXPECT_EQ(tokens[0].type, TokenType::KW_LET);
    EXPECT_EQ(tokens[1].type, TokenType::KW_PRINT);
    EXPECT_EQ(tokens[2].type, TokenType::KW_INPUT);
    EXPECT_EQ(tokens[3].type, TokenType::KW_GOTO);
    EXPECT_EQ(tokens[4].type, TokenType::KW_GOSUB);
    EXPECT_EQ(tokens[5].type, TokenType::KW_RETURN);
    EXPECT_EQ(tokens[6].type, TokenType::KW_IF);
    EXPECT_EQ(tokens[7].type, TokenType::KW_THEN);
    EXPECT_EQ(tokens[8].type, TokenType::KW_FOR);
    EXPECT_EQ(tokens[9].type, TokenType::KW_TO);
    EXPECT_EQ(tokens[10].type, TokenType::KW_STEP);
    EXPECT_EQ(tokens[11].type, TokenType::KW_NEXT);
    EXPECT_EQ(tokens[12].type, TokenType::KW_DIM);
    EXPECT_EQ(tokens[13].type, TokenType::KW_END);
    EXPECT_EQ(tokens[14].type, TokenType::KW_REM);
}

TEST(Lexer, LogicalKeywords) {
    auto tokens = toks("AND OR NOT");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type, TokenType::KW_AND);
    EXPECT_EQ(tokens[1].type, TokenType::KW_OR);
    EXPECT_EQ(tokens[2].type, TokenType::KW_NOT);
}

TEST(Lexer, KeywordsAreCaseInsensitive) {
    auto tokens = toks("print Print PRINT");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type, TokenType::KW_PRINT);
    EXPECT_EQ(tokens[1].type, TokenType::KW_PRINT);
    EXPECT_EQ(tokens[2].type, TokenType::KW_PRINT);
}

// --- Operators and delimiters ---

TEST(Lexer, ArithmeticOperators) {
    auto tokens = toks("+ - * / ^");
    ASSERT_EQ(tokens.size(), 5u);
    EXPECT_EQ(tokens[0].type, TokenType::PLUS);
    EXPECT_EQ(tokens[1].type, TokenType::MINUS);
    EXPECT_EQ(tokens[2].type, TokenType::STAR);
    EXPECT_EQ(tokens[3].type, TokenType::SLASH);
    EXPECT_EQ(tokens[4].type, TokenType::CARET);
}

TEST(Lexer, ComparisonOperators) {
    auto tokens = toks("= <> < > <= >=");
    ASSERT_EQ(tokens.size(), 6u);
    EXPECT_EQ(tokens[0].type, TokenType::EQUALS);
    EXPECT_EQ(tokens[1].type, TokenType::NOT_EQUAL);
    EXPECT_EQ(tokens[2].type, TokenType::LESS);
    EXPECT_EQ(tokens[3].type, TokenType::GREATER);
    EXPECT_EQ(tokens[4].type, TokenType::LESS_EQUAL);
    EXPECT_EQ(tokens[5].type, TokenType::GREATER_EQUAL);
}

TEST(Lexer, Delimiters) {
    auto tokens = toks("( ) , ;");
    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokens[0].type, TokenType::LPAREN);
    EXPECT_EQ(tokens[1].type, TokenType::RPAREN);
    EXPECT_EQ(tokens[2].type, TokenType::COMMA);
    EXPECT_EQ(tokens[3].type, TokenType::SEMICOLON);
}

// --- Literals ---

TEST(Lexer, IntegerLiterals) {
    auto tokens = toks("0 42 1000");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[0].lexeme, "0");
    EXPECT_EQ(tokens[1].lexeme, "42");
    EXPECT_EQ(tokens[2].lexeme, "1000");
}

TEST(Lexer, FloatLiterals) {
    auto tokens = toks("3.14 0.5 100.0");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type, TokenType::FLOAT_LITERAL);
    EXPECT_EQ(tokens[0].lexeme, "3.14");
    EXPECT_EQ(tokens[1].lexeme, "0.5");
    EXPECT_EQ(tokens[2].lexeme, "100.0");
}

TEST(Lexer, StringLiterals) {
    auto tokens = toks("\"hello\" \"world\"");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].lexeme, "hello");
    EXPECT_EQ(tokens[1].lexeme, "world");
}

TEST(Lexer, EmptyString) {
    auto tokens = toks("\"\"");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].lexeme, "");
}

// --- Identifiers ---

TEST(Lexer, NumericIdentifiers) {
    auto tokens = toks("X COUNT A1");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].lexeme, "X");
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].lexeme, "COUNT");
    EXPECT_EQ(tokens[2].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].lexeme, "A1");
}

TEST(Lexer, StringIdentifiers) {
    auto tokens = toks("N$ LINE$ A$");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type, TokenType::STRING_IDENTIFIER);
    EXPECT_EQ(tokens[0].lexeme, "N$");
    EXPECT_EQ(tokens[1].type, TokenType::STRING_IDENTIFIER);
    EXPECT_EQ(tokens[1].lexeme, "LINE$");
    EXPECT_EQ(tokens[2].type, TokenType::STRING_IDENTIFIER);
    EXPECT_EQ(tokens[2].lexeme, "A$");
}

// --- Multi-line programs ---

TEST(Lexer, MultiLineProgram) {
    auto tokens = toks(
        "10 PRINT \"HELLO\"\n"
        "20 END\n"
    );
    // Line 1: INTEGER PRINT STRING NEWLINE
    // Line 2: INTEGER END NEWLINE
    ASSERT_EQ(tokens.size(), 7u);
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[1].type, TokenType::KW_PRINT);
    EXPECT_EQ(tokens[2].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[3].type, TokenType::NEWLINE);
    EXPECT_EQ(tokens[4].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[5].type, TokenType::KW_END);
    EXPECT_EQ(tokens[6].type, TokenType::NEWLINE);
}

// --- REM eats rest of line ---

TEST(Lexer, RemEatsRestOfLine) {
    auto tokens = toks("10 REM this is a comment\n20 END");
    // REM should consume everything after it on that line
    ASSERT_GE(tokens.size(), 4u);
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[1].type, TokenType::KW_REM);
    // After REM, next meaningful token should be on the next line
    // REM's comment text is stored in the REM token's lexeme
    EXPECT_EQ(tokens[1].lexeme, "this is a comment");
    EXPECT_EQ(tokens[2].type, TokenType::NEWLINE);
    EXPECT_EQ(tokens[3].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[3].lexeme, "20");
}

// --- Complex expression ---

TEST(Lexer, ComplexExpression) {
    auto tokens = toks("LET X = (A + B) * 2 - C / 3");
    ASSERT_EQ(tokens.size(), 14u);
    EXPECT_EQ(tokens[0].type, TokenType::KW_LET);
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].type, TokenType::EQUALS);
    EXPECT_EQ(tokens[3].type, TokenType::LPAREN);
    EXPECT_EQ(tokens[4].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[5].type, TokenType::PLUS);
    EXPECT_EQ(tokens[6].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[7].type, TokenType::RPAREN);
    EXPECT_EQ(tokens[8].type, TokenType::STAR);
    EXPECT_EQ(tokens[9].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[10].type, TokenType::MINUS);
    EXPECT_EQ(tokens[11].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[12].type, TokenType::SLASH);
    EXPECT_EQ(tokens[13].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[13].lexeme, "3");
}

// --- GPU keywords (v2, but lexer should handle them now) ---

TEST(Lexer, GpuKeywords) {
    auto tokens = toks("GPU KERNEL COPY FREE WITH BLOCKS OF THREAD_IDX BLOCK_IDX BLOCK_DIM GRID_DIM");
    ASSERT_EQ(tokens.size(), 11u);
    EXPECT_EQ(tokens[0].type, TokenType::KW_GPU);
    EXPECT_EQ(tokens[1].type, TokenType::KW_KERNEL);
    EXPECT_EQ(tokens[2].type, TokenType::KW_COPY);
    EXPECT_EQ(tokens[3].type, TokenType::KW_FREE);
    EXPECT_EQ(tokens[4].type, TokenType::KW_WITH);
    EXPECT_EQ(tokens[5].type, TokenType::KW_BLOCKS);
    EXPECT_EQ(tokens[6].type, TokenType::KW_OF);
    EXPECT_EQ(tokens[7].type, TokenType::KW_THREAD_IDX);
    EXPECT_EQ(tokens[8].type, TokenType::KW_BLOCK_IDX);
    EXPECT_EQ(tokens[9].type, TokenType::KW_BLOCK_DIM);
    EXPECT_EQ(tokens[10].type, TokenType::KW_GRID_DIM);
}

// --- Error cases ---

TEST(Lexer, UnterminatedString) {
    Lexer lexer("10 PRINT \"hello");
    EXPECT_THROW(lexer.tokenize(), std::runtime_error);
}

TEST(Lexer, UnexpectedCharacter) {
    Lexer lexer("10 PRINT @");
    EXPECT_THROW(lexer.tokenize(), std::runtime_error);
}
