#pragma once

#include <vector>
#include "token.h"

namespace rocbas {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    void parse();

private:
    std::vector<Token> tokens_;
};

} // namespace rocbas
