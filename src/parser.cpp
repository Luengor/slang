#include "parser.hpp"
#include "ast.hpp"
#include "error.hpp"
#include <cassert>
#include <iostream>
#include <vector>

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

void Parser::syncronize() {
    this->advance();

    while (!this->isAtEnd()) {
        if (this->previous().type == Token::Type::Semicolon)
            return;

        switch (this->peek().type) {
            case Token::Type::Fixed:
            case Token::Type::Float:
            case Token::Type::Bool:
            case Token::Type::Auto:
            case Token::Type::If:
                return;
            default:
                break;
        }

        this->advance();
    }
}

/// Recursive descent parsing methods

std::unique_ptr<ASTNode> Parser::declaration() {
    try {
        if (this->match({Token::Type::Fixed, Token::Type::Float,
                         Token::Type::Bool, Token::Type::Auto})) {
            return this->varDecl();
        }

        return this->statement();
    } catch (const ParserError &error) {
        std::cerr << error.what() << std::endl;
        this->syncronize();
        return nullptr;
    }
}

std::unique_ptr<ASTNode> Parser::varDecl() {
    Token type_token = this->previous();
    Token name = this->consume(Token::Type::Identifier, "Expected variable name.");

    std::unique_ptr<ASTNode> initializer = nullptr;
    if (match({Token::Type::Equal})) {
        initializer = this->expression();
    }

    this->consume(Token::Type::Semicolon, "Expected ';' after variable declaration.");
    return std::make_unique<VarDeclStmt>(type_token, name,
                                         std::move(initializer));
}

std::unique_ptr<ASTNode> Parser::statement() {
    if (this->match({Token::Type::LeftBrace})) {
        return this->block();
    } else if (this->match({Token::Type::If})) {
        return this->ifStmt();
    }

    return this->exprStmt();
}

std::unique_ptr<ASTNode> Parser::exprStmt() {
    std::unique_ptr<ASTNode> expr = this->expression();
    this->consume(Token::Type::Semicolon, "Expected ';' after expression.");
    return std::make_unique<ExprStmt>(expr->token, std::move(expr));
}

std::unique_ptr<ASTNode> Parser::ifStmt() {
    Token ifToken = this->previous();
    this->consume(Token::Type::LeftParen, "Expected '(' after 'if'.");
    std::unique_ptr<ASTNode> condition = this->expression();
    this->consume(Token::Type::RightParen, "Expected ')' after if condition.");

    std::unique_ptr<ASTNode> thenBranch = this->statement();
    std::unique_ptr<ASTNode> elseBranch = nullptr;

    if (this->match({Token::Type::Else})) {
        elseBranch = this->statement();
    }

    return std::make_unique<IfStmt>(
        ifToken, std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<ASTNode> Parser::block() {
    std::vector<ASTNodePtr> statements;

    while (!this->check(Token::Type::RightBrace) && !this->isAtEnd()) {
        statements.push_back(this->declaration());
    }

    Token braceToken = this->consume(
        Token::Type::RightBrace, "Expected '}' after block.");

    return std::make_unique<BlockStmt>(braceToken, std::move(statements));
}

std::unique_ptr<ASTNode> Parser::expression() {
    return this->assignment();
}

std::unique_ptr<ASTNode> Parser::assignment() {
    std::unique_ptr<ASTNode> expr = this->equality();

    if (this->match({Token::Type::Equal})) {
        Token equals = this->previous();
        std::unique_ptr<ASTNode> value = this->assignment();

        // Perform assignment only if the left side is a VariableNode
        if (expr->type != ASTNodeType::Variable) {
            throw ParserError(equals, "Invalid assignment target.");
        }

        return std::make_unique<AssignExpr>(
            equals, std::move(expr), std::move(value));
    }

    return expr;
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
        if (operatorToken.type == Token::Type::Or) 
            expr = std::make_unique<LogicExpr>(
                operatorToken, std::move(expr), std::move(right));
        else
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
        if (operatorToken.type == Token::Type::And) 
            expr = std::make_unique<LogicExpr>(
                operatorToken, std::move(expr), std::move(right));
        else
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

    // Make a variable node for identifiers
    if (this->match({Token::Type::Identifier})) {
        return std::make_unique<VariableNode>(
            this->previous(), this->previous().lexeme);
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
    std::vector<ASTNodePtr> statements;
    bool failed = false;

    while (!this->isAtEnd()) {
        auto stmt = this->declaration();
        if (stmt != nullptr) {
            statements.push_back(std::move(stmt));
        } else {
            failed = true;
        }
    }

    if (failed) {
        return nullptr;
    }

    return std::make_unique<BlockStmt>(Token{}, std::move(statements));
}
