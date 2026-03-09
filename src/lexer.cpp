#include "lexer.h"
#include <stdexcept>

namespace rocbas {

Lexer::Lexer(const std::string& source) : source_(source) {}

std::vector<Token> Lexer::tokenize() {
    throw std::runtime_error("Lexer::tokenize() not yet implemented");
}

} // namespace rocbas
