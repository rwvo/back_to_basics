#include "lexer.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unordered_map>

namespace rocbas {

static const std::unordered_map<std::string, TokenType> keywords = {
    {"LET",        TokenType::KW_LET},
    {"PRINT",      TokenType::KW_PRINT},
    {"INPUT",      TokenType::KW_INPUT},
    {"GOTO",       TokenType::KW_GOTO},
    {"GOSUB",      TokenType::KW_GOSUB},
    {"RETURN",     TokenType::KW_RETURN},
    {"IF",         TokenType::KW_IF},
    {"THEN",       TokenType::KW_THEN},
    {"FOR",        TokenType::KW_FOR},
    {"TO",         TokenType::KW_TO},
    {"STEP",       TokenType::KW_STEP},
    {"NEXT",       TokenType::KW_NEXT},
    {"DIM",        TokenType::KW_DIM},
    {"END",        TokenType::KW_END},
    {"REM",        TokenType::KW_REM},
    {"DATA",       TokenType::KW_DATA},
    {"READ",       TokenType::KW_READ},
    {"RESTORE",    TokenType::KW_RESTORE},
    {"AND",        TokenType::KW_AND},
    {"OR",         TokenType::KW_OR},
    {"NOT",        TokenType::KW_NOT},
    {"GPU",        TokenType::KW_GPU},
    {"KERNEL",     TokenType::KW_KERNEL},
    {"LAUNCH",     TokenType::KW_LAUNCH},
    {"COPY",       TokenType::KW_COPY},
    {"FREE",       TokenType::KW_FREE},
    {"WITH",       TokenType::KW_WITH},
    {"BLOCKS",     TokenType::KW_BLOCKS},
    {"OF",         TokenType::KW_OF},
    {"THREAD_ID",  TokenType::KW_THREAD_ID},
    {"BLOCK_ID",   TokenType::KW_BLOCK_ID},
    {"BLOCK_SIZE", TokenType::KW_BLOCK_SIZE},
    {"GRID_SIZE",  TokenType::KW_GRID_SIZE},
};

std::ostream& operator<<(std::ostream& os, TokenType type) {
    switch (type) {
        case TokenType::INTEGER_LITERAL:   return os << "INTEGER_LITERAL";
        case TokenType::FLOAT_LITERAL:     return os << "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL:    return os << "STRING_LITERAL";
        case TokenType::IDENTIFIER:        return os << "IDENTIFIER";
        case TokenType::STRING_IDENTIFIER: return os << "STRING_IDENTIFIER";
        case TokenType::PLUS:              return os << "PLUS";
        case TokenType::MINUS:             return os << "MINUS";
        case TokenType::STAR:              return os << "STAR";
        case TokenType::SLASH:             return os << "SLASH";
        case TokenType::CARET:             return os << "CARET";
        case TokenType::EQUALS:            return os << "EQUALS";
        case TokenType::NOT_EQUAL:         return os << "NOT_EQUAL";
        case TokenType::LESS:              return os << "LESS";
        case TokenType::GREATER:           return os << "GREATER";
        case TokenType::LESS_EQUAL:        return os << "LESS_EQUAL";
        case TokenType::GREATER_EQUAL:     return os << "GREATER_EQUAL";
        case TokenType::LPAREN:            return os << "LPAREN";
        case TokenType::RPAREN:            return os << "RPAREN";
        case TokenType::COMMA:             return os << "COMMA";
        case TokenType::SEMICOLON:         return os << "SEMICOLON";
        case TokenType::KW_LET:            return os << "KW_LET";
        case TokenType::KW_PRINT:          return os << "KW_PRINT";
        case TokenType::KW_INPUT:          return os << "KW_INPUT";
        case TokenType::KW_GOTO:           return os << "KW_GOTO";
        case TokenType::KW_GOSUB:          return os << "KW_GOSUB";
        case TokenType::KW_RETURN:         return os << "KW_RETURN";
        case TokenType::KW_IF:             return os << "KW_IF";
        case TokenType::KW_THEN:           return os << "KW_THEN";
        case TokenType::KW_FOR:            return os << "KW_FOR";
        case TokenType::KW_TO:             return os << "KW_TO";
        case TokenType::KW_STEP:           return os << "KW_STEP";
        case TokenType::KW_NEXT:           return os << "KW_NEXT";
        case TokenType::KW_DIM:            return os << "KW_DIM";
        case TokenType::KW_END:            return os << "KW_END";
        case TokenType::KW_REM:            return os << "KW_REM";
        case TokenType::KW_DATA:           return os << "KW_DATA";
        case TokenType::KW_READ:           return os << "KW_READ";
        case TokenType::KW_RESTORE:        return os << "KW_RESTORE";
        case TokenType::KW_AND:            return os << "KW_AND";
        case TokenType::KW_OR:             return os << "KW_OR";
        case TokenType::KW_NOT:            return os << "KW_NOT";
        case TokenType::KW_GPU:            return os << "KW_GPU";
        case TokenType::KW_KERNEL:         return os << "KW_KERNEL";
        case TokenType::KW_LAUNCH:         return os << "KW_LAUNCH";
        case TokenType::KW_COPY:           return os << "KW_COPY";
        case TokenType::KW_FREE:           return os << "KW_FREE";
        case TokenType::KW_WITH:           return os << "KW_WITH";
        case TokenType::KW_BLOCKS:         return os << "KW_BLOCKS";
        case TokenType::KW_OF:             return os << "KW_OF";
        case TokenType::KW_THREAD_ID:      return os << "KW_THREAD_ID";
        case TokenType::KW_BLOCK_ID:       return os << "KW_BLOCK_ID";
        case TokenType::KW_BLOCK_SIZE:     return os << "KW_BLOCK_SIZE";
        case TokenType::KW_GRID_SIZE:      return os << "KW_GRID_SIZE";
        case TokenType::NEWLINE:           return os << "NEWLINE";
        case TokenType::END_OF_FILE:       return os << "END_OF_FILE";
    }
    return os << "UNKNOWN(" << static_cast<int>(type) << ")";
}

Lexer::Lexer(const std::string& source) : source_(source) {}

char Lexer::peek() const {
    if (at_end()) return '\0';
    return source_[pos_];
}

char Lexer::peek_next() const {
    if (pos_ + 1 >= source_.size()) return '\0';
    return source_[pos_ + 1];
}

char Lexer::advance() {
    char c = source_[pos_++];
    if (c == '\n') source_line_++;
    return c;
}

bool Lexer::at_end() const {
    return pos_ >= source_.size();
}

void Lexer::skip_whitespace() {
    while (!at_end() && (peek() == ' ' || peek() == '\t' || peek() == '\r')) {
        advance();
    }
}

Token Lexer::make_token(TokenType type, const std::string& lexeme) const {
    return Token{type, lexeme, 0, source_line_};
}

Token Lexer::scan_number() {
    size_t start = pos_;
    while (!at_end() && std::isdigit(peek())) {
        advance();
    }
    // Check for decimal point
    if (!at_end() && peek() == '.' && std::isdigit(peek_next())) {
        advance(); // consume '.'
        while (!at_end() && std::isdigit(peek())) {
            advance();
        }
        return make_token(TokenType::FLOAT_LITERAL, source_.substr(start, pos_ - start));
    }
    return make_token(TokenType::INTEGER_LITERAL, source_.substr(start, pos_ - start));
}

Token Lexer::scan_string() {
    advance(); // consume opening quote
    size_t start = pos_;
    while (!at_end() && peek() != '"' && peek() != '\n') {
        advance();
    }
    if (at_end() || peek() == '\n') {
        throw std::runtime_error("Unterminated string literal at line " +
                                 std::to_string(source_line_));
    }
    std::string value = source_.substr(start, pos_ - start);
    advance(); // consume closing quote
    return make_token(TokenType::STRING_LITERAL, value);
}

Token Lexer::scan_identifier_or_keyword() {
    size_t start = pos_;
    while (!at_end() && (std::isalnum(peek()) || peek() == '_')) {
        advance();
    }
    // Check for string variable suffix ($)
    if (!at_end() && peek() == '$') {
        advance();
        std::string lexeme = source_.substr(start, pos_ - start);
        // Uppercase it for canonical form
        std::string upper = lexeme;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        return make_token(TokenType::STRING_IDENTIFIER, upper);
    }

    std::string lexeme = source_.substr(start, pos_ - start);
    // Uppercase for keyword lookup (BASIC is case-insensitive)
    std::string upper = lexeme;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    auto it = keywords.find(upper);
    if (it != keywords.end()) {
        return make_token(it->second, upper);
    }
    // Regular identifier — store uppercase
    return make_token(TokenType::IDENTIFIER, upper);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!at_end()) {
        skip_whitespace();
        if (at_end()) break;

        char c = peek();

        if (c == '\n') {
            advance();
            tokens.push_back(make_token(TokenType::NEWLINE, "\\n"));
            continue;
        }

        if (std::isdigit(c)) {
            tokens.push_back(scan_number());
            continue;
        }

        if (c == '"') {
            tokens.push_back(scan_string());
            continue;
        }

        if (std::isalpha(c) || c == '_') {
            Token tok = scan_identifier_or_keyword();
            // If REM, consume rest of line as comment
            if (tok.type == TokenType::KW_REM) {
                skip_whitespace();
                size_t start = pos_;
                while (!at_end() && peek() != '\n') {
                    advance();
                }
                // Trim trailing whitespace from comment
                size_t end = pos_;
                while (end > start && (source_[end - 1] == ' ' || source_[end - 1] == '\t')) {
                    end--;
                }
                tok.lexeme = source_.substr(start, end - start);
                tokens.push_back(tok);
                continue;
            }
            tokens.push_back(tok);
            continue;
        }

        // Single and multi-character operators
        advance();
        switch (c) {
            case '+': tokens.push_back(make_token(TokenType::PLUS, "+")); break;
            case '-': tokens.push_back(make_token(TokenType::MINUS, "-")); break;
            case '*': tokens.push_back(make_token(TokenType::STAR, "*")); break;
            case '/': tokens.push_back(make_token(TokenType::SLASH, "/")); break;
            case '^': tokens.push_back(make_token(TokenType::CARET, "^")); break;
            case '(': tokens.push_back(make_token(TokenType::LPAREN, "(")); break;
            case ')': tokens.push_back(make_token(TokenType::RPAREN, ")")); break;
            case ',': tokens.push_back(make_token(TokenType::COMMA, ",")); break;
            case ';': tokens.push_back(make_token(TokenType::SEMICOLON, ";")); break;
            case '=': tokens.push_back(make_token(TokenType::EQUALS, "=")); break;
            case '<':
                if (!at_end() && peek() == '>') {
                    advance();
                    tokens.push_back(make_token(TokenType::NOT_EQUAL, "<>"));
                } else if (!at_end() && peek() == '=') {
                    advance();
                    tokens.push_back(make_token(TokenType::LESS_EQUAL, "<="));
                } else {
                    tokens.push_back(make_token(TokenType::LESS, "<"));
                }
                break;
            case '>':
                if (!at_end() && peek() == '=') {
                    advance();
                    tokens.push_back(make_token(TokenType::GREATER_EQUAL, ">="));
                } else {
                    tokens.push_back(make_token(TokenType::GREATER, ">"));
                }
                break;
            default:
                throw std::runtime_error("Unexpected character '" + std::string(1, c) +
                                         "' at line " + std::to_string(source_line_));
        }
    }

    tokens.push_back(make_token(TokenType::END_OF_FILE, ""));
    return tokens;
}

} // namespace rocbas
