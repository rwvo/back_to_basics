#pragma once

#include <string>
#include <ostream>

namespace rocbas {

enum class TokenType {
    // Literals
    INTEGER_LITERAL,    // 42
    FLOAT_LITERAL,      // 3.14
    STRING_LITERAL,     // "hello"

    // Identifiers
    IDENTIFIER,         // variable names: X, COUNT
    STRING_IDENTIFIER,  // string variable names: N$, LINE$

    // Arithmetic operators
    PLUS,               // +
    MINUS,              // -
    STAR,               // *
    SLASH,              // /
    CARET,              // ^

    // Comparison operators
    EQUALS,             // =
    NOT_EQUAL,          // <>
    LESS,               // <
    GREATER,            // >
    LESS_EQUAL,         // <=
    GREATER_EQUAL,      // >=

    // Delimiters
    LPAREN,             // (
    RPAREN,             // )
    COMMA,              // ,
    SEMICOLON,          // ;

    // Keywords — BASIC statements
    KW_LET,
    KW_PRINT,
    KW_INPUT,
    KW_GOTO,
    KW_GOSUB,
    KW_RETURN,
    KW_IF,
    KW_THEN,
    KW_FOR,
    KW_TO,
    KW_STEP,
    KW_NEXT,
    KW_DIM,
    KW_END,
    KW_REM,
    KW_DATA,
    KW_READ,
    KW_RESTORE,

    // Keywords — logical operators
    KW_AND,
    KW_OR,
    KW_NOT,

    // Keywords — GPU extensions (v2)
    KW_GPU,
    KW_KERNEL,
    KW_COPY,
    KW_FREE,
    KW_WITH,
    KW_BLOCKS,
    KW_OF,
    KW_THREAD_IDX,
    KW_BLOCK_IDX,
    KW_BLOCK_DIM,
    KW_GRID_DIM,
    KW_OPTION,
    KW_BASE,

    // Keywords — MPI extensions (v3)
    KW_MPI,
    KW_SEND,
    KW_RECV,
    KW_FROM,
    KW_TAG,
    KW_THRU,
    KW_BARRIER,
    KW_INIT,
    KW_FINALIZE,

    // Special
    NEWLINE,            // end of a line (logical separator)
    END_OF_FILE,
};

struct Token {
    TokenType type = TokenType::END_OF_FILE;
    std::string lexeme;
    int line_number = 0;    // the BASIC line number (e.g., 10, 20, 30)
    int source_line = 0;    // the source file line (for error reporting)
};

// For GTest pretty-printing
std::ostream& operator<<(std::ostream& os, TokenType type);

} // namespace rocbas
