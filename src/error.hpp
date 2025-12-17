#pragma once

#include "scanner.hpp"
#include <stdexcept>

class ParserError : public std::runtime_error {
    static std::string makeError(const Token &token, const char *message);

  public:
    ParserError(const Token &token, const char *message);
};
