#include "ast_core.hpp"
#include "ast_expr.hpp"
#include "ast_stmt.hpp"
#include "ast_type.hpp"
#include "parser.hpp"
#include "error.hpp"
#include <cassert>
#include <iostream>
#include <memory>
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
            case Token::Type::While:
            case Token::Type::For:
                return;
            default:
                break;
        }

        this->advance();
    }
}

bool Parser::isFunctionTypeAhead() {
    size_t temp_current = this->current;

    // Check if there is an opening parenthesis
    if (!this->check(Token::Type::LeftParen))
        return false;
    this->advance();

    // Find the matching closing parenthesis
    int paren_count = 0;
    do {
        if (this->isAtEnd()) {
            this->current = temp_current; // Restore current
            return false;
        }

        if (this->check(Token::Type::LeftParen)) {
            paren_count++;
        } else if (this->check(Token::Type::RightParen)) {
            if (paren_count == 0) {
                this->advance(); // Consume the closing parenthesis
                break;
            } else {
                paren_count--;
            }
        }
        this->advance();
    } while (true);

    // Check for an arrow or an identifier next
    bool is_function_type = this->check(Token::Type::Arrow) ||
                            this->check(Token::Type::Identifier);

    this->current = temp_current; // Restore current
    return is_function_type;
}

/// Recursive descent parsing methods

std::unique_ptr<ASTNode> Parser::typeExpr() {
    // Try primitive type first
    if (this->match({Token::Type::Fixed, Token::Type::Float,
                         Token::Type::Bool})) {
        return this->primitiveType();
    }

    // Then try function type
    return this->functionType();
}

std::unique_ptr<ASTNode> Parser::functionType() {
    // Start consuming the function type
    this->consume(Token::Type::LeftParen,
                  "Expected '(' at start of function type.");

    std::vector<ASTNodePtr> param_types;
    if (!this->check(Token::Type::RightParen)) {
        do {
            param_types.push_back(this->typeExpr());
        } while (this->match({Token::Type::Comma}));
    }

    this->consume(Token::Type::RightParen,
                  "Expected ')' after function parameter types.");
    this->consume(Token::Type::Arrow,
                  "Expected '->' after function parameter list.");

    // Return typeExpr or none
    std::unique_ptr<ASTNode> return_type;
    if (this->match({Token::Type::None})) {
        return_type = std::make_unique<PrimitiveTypeNode>(this->previous());
    } else {
        return_type = this->typeExpr();
    }

    return std::make_unique<FunctionTypeNode>(
        this->previous(), std::move(param_types), std::move(return_type));
}

std::unique_ptr<ASTNode> Parser::primitiveType() {
    Token type_token = this->previous();
    return std::make_unique<PrimitiveTypeNode>(type_token);
}

std::unique_ptr<ASTNode> Parser::declaration() {
    try {
        // Check if its a function declaration first
        if (this->isFunctionTypeAhead()) {
            return this->funcDecl();
        } else if (this->match({Token::Type::Fixed, Token::Type::Float,
                                Token::Type::Bool, Token::Type::Auto})) {
            // This is a variable declaration
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
    std::unique_ptr<ASTNode> type_node =
        this->previous().type == Token::Type::Auto ? nullptr
                                                   : this->primitiveType();
    Token name = this->consume(Token::Type::Identifier, "Expected variable name.");

    std::unique_ptr<ASTNode> initializer = nullptr;
    if (match({Token::Type::Equal})) {
        initializer = this->expression();
    }

    this->consume(Token::Type::Semicolon, "Expected ';' after variable declaration.");
    return std::make_unique<VarDeclStmt>(std::move(type_node), name,
                                         std::move(initializer));
}

std::unique_ptr<ASTNode> Parser::funcDecl() {
    std::unique_ptr<ASTNode> type_node = this->functionType();
    Token name = this->consume(Token::Type::Identifier, "Expected function name.");
    this->consume(Token::Type::Equal, "Expected '=' after function declaration.");

    // For now, we won't parse the function body
    type_node->print();
    throw ParserError(this->peek(), "Function body parsing not implemented yet.");
}

std::unique_ptr<ASTNode> Parser::statement() {
    if (this->match({Token::Type::LeftBrace})) {
        return this->block();
    } else if (this->match({Token::Type::If})) {
        return this->ifStmt();
    } else if (this->match({Token::Type::While})) {
        return this->whileStmt();
    } else if (this->match({Token::Type::For})) {
        return this->forStmt();
    } else if (this->match({Token::Type::Semicolon})) {
        // Empty statement
        Token semicolonToken = this->previous();
        return std::make_unique<ExprStmt>(semicolonToken, nullptr);
    }

    return this->exprStmt();
}

std::unique_ptr<ASTNode> Parser::exprStmt() {
    std::unique_ptr<ASTNode> expr = this->expression();
    this->consume(Token::Type::Semicolon, "Expected ';' after expression.");
    return std::make_unique<ExprStmt>(this->previous(), std::move(expr));
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

std::unique_ptr<ASTNode> Parser::whileStmt() {
    Token while_token = this->previous();
    this->consume(Token::Type::LeftParen, "Expected '(' after 'while'.");
    std::unique_ptr<ASTNode> condition = this->expression();
    this->consume(Token::Type::RightParen, "Expected ')' after while condition.");

    std::unique_ptr<ASTNode> body = this->statement();

    return std::make_unique<WhileStmt>(
        while_token, std::move(condition), std::move(body));
}

std::unique_ptr<ASTNode> Parser::forStmt() {
    Token for_token = this->previous();
    this->consume(Token::Type::LeftParen, "Expected '(' after 'for'.");
    std::unique_ptr<ASTNode> initializer = nullptr;

    if (!this->match({Token::Type::Semicolon})) {
        if (this->match({Token::Type::Fixed, Token::Type::Float,
                         Token::Type::Bool, Token::Type::Auto})) {
            initializer = this->varDecl();
        } else {
            initializer = this->exprStmt();
        }
    }

    std::unique_ptr<ASTNode> condition = nullptr;
    if (!this->match({Token::Type::Semicolon})) {
        condition = this->expression();
        this->consume(Token::Type::Semicolon, "Expected ';' after loop condition.");
    }

    std::unique_ptr<ASTNode> increment = nullptr;
    if (!this->match({Token::Type::RightParen})) {
        increment = this->expression();
        this->consume(Token::Type::RightParen, "Expected ')' after for clauses.");
    }

    std::unique_ptr<ASTNode> body = this->statement();

    // caramelize for loop into while loop
    // Prepare a secuence of statements for the block
    std::vector<ASTNodePtr> statements;

    // If there's an initializer, add it first
    if (initializer) {
        statements.push_back(std::move(initializer));
    }

    // Add the increment clause to the end of the body
    if (increment) {
        std::vector<ASTNodePtr> body_statements;
        body_statements.push_back(std::move(body));

        // The increment should be an expression statement to pop its result
        body_statements.push_back(
            std::make_unique<ExprStmt>(increment->token, std::move(increment)));
        body = std::make_unique<BlockStmt>(for_token, std::move(body_statements));
    }

    // If no condition, use 'true' literal
    if (!condition) {
        Token trueToken{
            .type = Token::Type::True,
            .lexeme = "true",
            .line = for_token.line
        };
        condition = std::make_unique<LiteralNode>(trueToken);
    }

    // Create the while statement
    std::unique_ptr<ASTNode> whileStmt = std::make_unique<WhileStmt>(
        for_token, std::move(condition), std::move(body));

    statements.push_back(std::move(whileStmt));

    return std::make_unique<BlockStmt>(for_token, std::move(statements));
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
    std::unique_ptr<ASTNode> expr = this->logicOr();

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

std::unique_ptr<ASTNode> Parser::logicOr() {
    // Get the left operand
    std::unique_ptr<ASTNode> expr = this->logicAnd();

    // While the current token is 'or', parse the right operand
    while (this->match({Token::Type::Or})) {
        Token operatorToken = this->previous();
        std::unique_ptr<ASTNode> right = this->logicAnd();

        // Create a logic expression node
        expr = std::make_unique<LogicExpr>(
            operatorToken, std::move(expr), std::move(right));
    }

    return expr;
}

std::unique_ptr<ASTNode> Parser::logicAnd() {
    // Get the left operand
    std::unique_ptr<ASTNode> expr = this->equality();

    // While the current token is 'and', parse the right operand
    while (this->match({Token::Type::And})) {
        Token operatorToken = this->previous();
        std::unique_ptr<ASTNode> right = this->equality();

        // Create a logic expression node
        expr = std::make_unique<LogicExpr>(
            operatorToken, std::move(expr), std::move(right));
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

    // While the current token is a '+' or '-', parse the right operand
    while (this->match({Token::Type::Plus, Token::Type::Minus})) {
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

    // While the current token is a '*' or '/', parse the right operand
    while (this->match({Token::Type::Star, Token::Type::Slash})) {
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
