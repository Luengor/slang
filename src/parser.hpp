#pragma once

#include "ast.hpp"
#include "scanner.hpp"
#include <memory>

enum class Precedence {
    None,
    Assignment,
    Or,
    And,
    Equality,
    Comparison,
    Term,
    Factor,
    Unary,
    Call,
    Primary
};

struct Parser {
    std::vector<Token>::const_iterator current, previous;
    std::unique_ptr<ASTNode> current_prefix = nullptr;

    void advance();
    void consume(Token::Type type, const std::string& message);

    std::unique_ptr<ASTNode> parsePrecedence(Precedence precedence);
    std::unique_ptr<ASTNode> expression();
    std::unique_ptr<ASTNode> unary();
    std::unique_ptr<ASTNode> binary();
    std::unique_ptr<ASTNode> number();
    std::unique_ptr<ASTNode> string();
    std::unique_ptr<ASTNode> grouping();

    std::unique_ptr<ASTNode> parse(const std::vector<Token>& tokens);
};

