#pragma once

#include "scanner.hpp"
#include <stdexcept>

class ParserError : public std::runtime_error {
    static std::string makeError(const Token &token,
                                 const std::string &message);

  public:
    ParserError(const Token &token, const std::string &message);
};
