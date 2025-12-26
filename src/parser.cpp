#include "parser.hpp"
#include "ast.hpp"
#include "error.hpp"
#include <cassert>
#include <iostream>

/// Parser helper methods

void Parser::advance() {
    if (!this->isAtEnd()) {
        ++this->current;
    }
}

const Token &Parser::peek() {
    assert(this->current < this->tokens.size());
    return this->tokens[this->current];
}

const Token &Parser::previous() {
    assert(this->current > 0 && this->current <= this->tokens.size());
    return this->tokens[this->current - 1];
}

bool Parser::isAtEnd() { return this->peek().type == Token::Type::Eof; }

bool Parser::check(Token::Type type) {
    if (this->isAtEnd())
        return false;
    return this->peek().type == type;
}

bool Parser::match(const std::initializer_list<Token::Type> &types) {
    for (const auto &type : types) {
        if (this->check(type)) {
            this->advance();
            return true;
        }
    }
    return false;
}

const Token &Parser::consume(Token::Type type, const char *message) {
    if (this->check(type)) {
        this->advance();
        return this->previous();
    }
    throw ParserError(this->peek(), message);
}

/// Recursive descent parsing methods

std::unique_ptr<ASTNode> Parser::expression() {
    return this->equality();
}

std::unique_ptr<ASTNode> Parser::equality() {
    // Get the left operand
    std::unique_ptr<ASTNode> expr = this->comparison();

    // While the current token is a '==' or '!=', parse the right operand
    while (this->match({Token::Type::EqualEqual, Token::Type::BangEqual})) {
        Token operatorToken = this->previous();
        std::unique_ptr<ASTNode> right = this->comparison();

        // Create a binary expression node
        expr = std::make_unique<BinaryExpr>(
            operatorToken, std::move(expr), std::move(right));
    }

    return expr;
}

std::unique_ptr<ASTNode> Parser::comparison() {
    // Get the left operand
    std::unique_ptr<ASTNode> expr = this->term();

    // While the current token is a comparison operator, parse the right operand
    while (this->match({Token::Type::Greater, Token::Type::GreaterEqual,
                       Token::Type::Less, Token::Type::LessEqual})) {
        Token operatorToken = this->previous();
        std::unique_ptr<ASTNode> right = this->term();

        // Create a binary expression node
        expr = std::make_unique<BinaryExpr>(
            operatorToken, std::move(expr), std::move(right));
    }

    return expr;
}

std::unique_ptr<ASTNode> Parser::term() {
    // Get the left operand
    std::unique_ptr<ASTNode> expr = this->factor();

    // While the current token is a '+', '-' or 'or', parse the right operand
    while (this->match({Token::Type::Plus, Token::Type::Minus, Token::Type::Or})) {
        Token operatorToken = this->previous();
        std::unique_ptr<ASTNode> right = this->factor();

        // Create a binary expression node
        expr = std::make_unique<BinaryExpr>(
            operatorToken, std::move(expr), std::move(right));
    }

    return expr;
}

std::unique_ptr<ASTNode> Parser::factor() {
    // Get the left operand
    std::unique_ptr<ASTNode> expr = this->unary();

    // While the current token is a '*', '/' or 'and', parse the right operand
    while (this->match({Token::Type::Star, Token::Type::Slash, Token::Type::And})) {
        Token operatorToken = this->previous();
        std::unique_ptr<ASTNode> right = this->unary();

        // Create a binary expression node
        expr = std::make_unique<BinaryExpr>(
            operatorToken, std::move(expr), std::move(right));
    }

    return expr;
}

std::unique_ptr<ASTNode> Parser::unary() {
    // Check for a unary operator
    if (this->match({Token::Type::Minus, Token::Type::Not})) {
        Token operatorToken = this->previous();
        std::unique_ptr<ASTNode> right = this->unary();
        return std::make_unique<UnaryExpr>(
            operatorToken, std::move(right));
    }

    // If no unary operator, parse a primary expression
    return this->primary();
}

std::unique_ptr<ASTNode> Parser::primary() {
    // Make a literal node for number, string, true, false
    if (this->match({Token::Type::Number, Token::Type::String,
                     Token::Type::True, Token::Type::False})) {
        return std::make_unique<LiteralNode>(this->previous());
    }

    // If left parenthesis, parse a grouped expression
    if (this->match({Token::Type::LeftParen})) {
        std::unique_ptr<ASTNode> expr = this->expression();
        this->consume(Token::Type::RightParen, "Expected ')' after expression.");
        return expr;
    }

    // Else, throw an error for unexpected token
    throw ParserError(this->peek(), "Expected expression.");
}

/// Public interface

Parser::Parser(const std::vector<Token> &tokens) : current(0), tokens(tokens) {}

std::unique_ptr<ASTNode> Parser::parse() {
    try {
        return this->expression();
    } catch (const ParserError &error) {
        std::cerr << error.what() << std::endl;
        return nullptr;
    }
}
