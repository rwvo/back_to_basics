#pragma once

#include <string>

// Token types and Token struct for the rocBAS lexer.
// Will be populated in Step 2.

namespace rocbas {

enum class TokenType {
    // Placeholder — real tokens added in Step 2
    END_OF_FILE,
};

struct Token {
    TokenType type = TokenType::END_OF_FILE;
    std::string lexeme;
    int line_number = 0;
};

} // namespace rocbas
