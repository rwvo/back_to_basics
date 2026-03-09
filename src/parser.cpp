#include "parser.h"
#include <stdexcept>

namespace rocbas {

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

void Parser::parse() {
    throw std::runtime_error("Parser::parse() not yet implemented");
}

} // namespace rocbas
