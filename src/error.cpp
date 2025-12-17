#include "error.hpp"
#include <format>

std::string ParserError::makeError(const Token &token, const char *message) {
    std::string errorMsg;
    errorMsg += std::format("[line {}] Error", token.line);

    if (token.type == Token::Type::Eof) {
        errorMsg += " at end";
    } else if (token.type != Token::Type::Error) {
        errorMsg += std::format(" at '{}'", token.lexeme);
    }

    errorMsg += std::format(": {}\n", message);
    return errorMsg;
}

ParserError::ParserError(const Token &token, const char *message)
    : std::runtime_error(ParserError::makeError(token, message)) {}
