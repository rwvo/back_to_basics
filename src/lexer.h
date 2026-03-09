#pragma once

#include <string>
#include <vector>
#include "token.h"

namespace rocbas {

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string source_;
    size_t pos_ = 0;
    int source_line_ = 1;

    char peek() const;
    char peek_next() const;
    char advance();
    bool at_end() const;
    void skip_whitespace();
    Token make_token(TokenType type, const std::string& lexeme) const;
    Token scan_number();
    Token scan_string();
    Token scan_identifier_or_keyword();
};

} // namespace rocbas
