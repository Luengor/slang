#pragma once

#include "ast.hpp"
#include "scanner.hpp"

class Parser {
public:
    Parser();
    ~Parser();

    ASTNode* parse(const std::vector<Token>& tokens);
};

