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
};

} // namespace rocbas
