#include "parser.hpp"
#include "ast_core.hpp"
#include "ast_expr.hpp"
#include "ast_pure_expr.hpp"
#include "ast_stmt.hpp"
#include "ast_type.hpp"
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
            case Token::Type::Str:
            case Token::Type::Float:
            case Token::Type::Bool:
            case Token::Type::Auto:
            case Token::Type::Alias:
            case Token::Type::If:
            case Token::Type::While:
            case Token::Type::For:
            case Token::Type::Return:
                return;
            default:
                break;
        }

        this->advance();
    }
}

/// Recursive descent parsing methods

ASTNodePtr Parser::typeExpr() {
    // Try primitive type first
    if (this->match({Token::Type::Fixed, Token::Type::Float, Token::Type::Bool,
                     Token::Type::Str})) {
        return this->primitiveType();
    }

    // Then try alias type
    if (this->match({Token::Type::Identifier})) {
        return this->aliasType();
    }

    // Then try function type
    return this->functionType();
}

ASTNodePtr Parser::aliasType() {
    Token name_token = this->previous();
    return std::make_unique<AliasTypeNode>(name_token, name_token.lexeme);
}

ASTNodePtr Parser::functionType() {
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
    ASTNodePtr return_type;
    if (this->match({Token::Type::None})) {
        return_type = std::make_unique<PrimitiveTypeNode>(this->previous());
    } else {
        return_type = this->typeExpr();
    }

    return std::make_unique<FunctionTypeNode>(
        this->previous(), std::move(param_types), std::move(return_type));
}

ASTNodePtr Parser::primitiveType() {
    Token type_token = this->previous();
    return std::make_unique<PrimitiveTypeNode>(type_token);
}

ASTNodePtr Parser::declaration() {
    try {
        if (this->match({Token::Type::Alias})) {
            return this->aliasDecl();
        }

        // Try variable declaration and backtrack on failure
        const auto current_pos = this->current;
        try {
            return this->varDecl();
        } catch (const ParserError &) {
            this->current = current_pos; // Backtrack
        }

        // Otherwise parse as a statement
        return this->statement();
    } catch (const ParserError &error) {
        std::cerr << error.what() << std::endl;
        this->syncronize();
        return nullptr;
    }
}

ASTNodePtr Parser::aliasDecl() {
    Token alias_name =
        this->consume(Token::Type::Identifier, "Expected alias name.");
    this->consume(Token::Type::Equal, "Expected '=' after alias name.");
    ASTNodePtr aliased_type = this->typeExpr();
    this->consume(Token::Type::Semicolon, "Expected ';' after alias declaration.");

    return std::make_unique<AliasDeclStmt>(alias_name, std::move(aliased_type));
}

ASTNodePtr Parser::varDecl() {
    // Parse type or auto
    ASTNodePtr type_node =
        this->match({Token::Type::Auto}) ? nullptr : this->typeExpr();
    Token name =
        this->consume(Token::Type::Identifier, "Expected variable name.");

    ASTNodePtr initializer = nullptr;
    if (match({Token::Type::Equal})) {
        initializer = this->expression();
    }

    this->consume(Token::Type::Semicolon,
                  "Expected ';' after variable declaration.");
    return std::make_unique<VarDeclStmt>(std::move(type_node), name,
                                         std::move(initializer));
}

ASTNodePtr Parser::statement() {
    if (this->match({Token::Type::LeftBrace})) {
        return this->block();
    } else if (this->match({Token::Type::If})) {
        return this->ifStmt();
    } else if (this->match({Token::Type::While})) {
        return this->whileStmt();
    } else if (this->match({Token::Type::For})) {
        return this->forStmt();
    } else if (this->match({Token::Type::Return})) {
        return this->returnStmt();
    } else if (this->match({Token::Type::Semicolon})) {
        // Empty statement
        Token semicolonToken = this->previous();
        return std::make_unique<ExprStmt>(semicolonToken, nullptr);
    }

    return this->exprStmt();
}

ASTNodePtr Parser::exprStmt() {
    ASTNodePtr expr = this->expression();
    this->consume(Token::Type::Semicolon, "Expected ';' after expression.");
    return std::make_unique<ExprStmt>(this->previous(), std::move(expr));
}

ASTNodePtr Parser::ifStmt() {
    Token ifToken = this->previous();
    this->consume(Token::Type::LeftParen, "Expected '(' after 'if'.");
    ASTNodePtr condition = this->expression();
    this->consume(Token::Type::RightParen, "Expected ')' after if condition.");

    ASTNodePtr thenBranch = this->statement();
    ASTNodePtr elseBranch = nullptr;

    if (this->match({Token::Type::Else})) {
        elseBranch = this->statement();
    }

    return std::make_unique<IfStmt>(ifToken, std::move(condition),
                                    std::move(thenBranch),
                                    std::move(elseBranch));
}

ASTNodePtr Parser::whileStmt() {
    Token while_token = this->previous();
    this->consume(Token::Type::LeftParen, "Expected '(' after 'while'.");
    ASTNodePtr condition = this->expression();
    this->consume(Token::Type::RightParen,
                  "Expected ')' after while condition.");

    ASTNodePtr body = this->statement();

    return std::make_unique<WhileStmt>(while_token, std::move(condition),
                                       std::move(body));
}

ASTNodePtr Parser::forStmt() {
    Token for_token = this->previous();
    this->consume(Token::Type::LeftParen, "Expected '(' after 'for'.");
    ASTNodePtr initializer = nullptr;

    if (!this->match({Token::Type::Semicolon})) {
        const auto current_pos = this->current;
        try {
            initializer = this->varDecl();
        } catch (const ParserError &) {
            this->current = current_pos;
            initializer = this->exprStmt();
        }
    }

    ASTNodePtr condition = nullptr;
    if (!this->match({Token::Type::Semicolon})) {
        condition = this->expression();
        this->consume(Token::Type::Semicolon,
                      "Expected ';' after loop condition.");
    }

    ASTNodePtr increment = nullptr;
    if (!this->match({Token::Type::RightParen})) {
        increment = this->expression();
        this->consume(Token::Type::RightParen,
                      "Expected ')' after for clauses.");
    }

    ASTNodePtr body = this->statement();

    // caramelize for loop into while loop
    // Prepare a secuence of statements for the block
    std::vector<ASTNodePtr> statements;

    // If there's an initializer, add it first
    if (initializer) {
        statements.push_back(std::move(initializer));
    }

    // Create a block for the body
    if (body->type == ASTNodeType::BlockStmt) {
        // The body is already a block. Add the increment clause if it exists
        if (increment) {
            auto block = static_cast<BlockStmt *>(body.get());
            block->statements.push_back(std::make_unique<ExprStmt>(
                increment->token, std::move(increment)));
        }
    } else {
        // Create a new block
        std::vector<ASTNodePtr> body_statements;
        body_statements.push_back(std::move(body));

        if (increment) {
            body_statements.push_back(std::make_unique<ExprStmt>(
                increment->token, std::move(increment)));
        }
        body =
            std::make_unique<BlockStmt>(for_token, std::move(body_statements));
    }

    // If no condition, use 'true' literal
    if (!condition) {
        Token trueToken{.type = Token::Type::True,
                        .lexeme = "true",
                        .line = for_token.line};
        condition = std::make_unique<LiteralNode>(trueToken);
    }

    // Create the while statement
    ASTNodePtr whileStmt = std::make_unique<WhileStmt>(
        for_token, std::move(condition), std::move(body));

    statements.push_back(std::move(whileStmt));

    return std::make_unique<BlockStmt>(for_token, std::move(statements));
}

ASTNodePtr Parser::returnStmt() {
    Token return_token = this->previous();

    // Get return expression if any
    ASTNodePtr expr = nullptr;
    if (!this->check(Token::Type::Semicolon))
        expr = this->expression();

    this->consume(Token::Type::Semicolon,
                  "Expected ';' after return statement.");
    return std::make_unique<ReturnStmt>(return_token, std::move(expr));
}

ASTNodePtr Parser::block() {
    std::vector<ASTNodePtr> statements;

    while (!this->check(Token::Type::RightBrace) && !this->isAtEnd()) {
        statements.push_back(this->declaration());
    }

    Token braceToken =
        this->consume(Token::Type::RightBrace, "Expected '}' after block.");

    return std::make_unique<BlockStmt>(braceToken, std::move(statements));
}

ASTNodePtr Parser::expression() { return this->assignment(); }

ASTNodePtr Parser::assignment() {
    ASTNodePtr expr = this->ternary();

    if (this->match({Token::Type::Equal})) {
        Token equals = this->previous();
        ASTNodePtr value = this->assignment();

        // Perform assignment only if the left side is a VariableNode
        if (expr->type != ASTNodeType::Variable) {
            throw ParserError(equals, "Invalid assignment target.");
        }

        return std::make_unique<AssignExpr>(equals, std::move(expr),
                                            std::move(value));
    }

    return expr;
}

ASTNodePtr Parser::ternary() {
    ASTNodePtr expr = this->logicOr();

    if (this->match({Token::Type::Question})) {
        Token questionToken = this->previous();
        ASTNodePtr thenExpr = this->expression();
        this->consume(Token::Type::Colon,
                      "Expected ':' in ternary expression.");
        ASTNodePtr elseExpr = this->expression();

        return std::make_unique<TernaryExpr>(questionToken, std::move(expr),
                                             std::move(thenExpr),
                                             std::move(elseExpr));
    }

    return expr;
}

ASTNodePtr Parser::logicOr() {
    // Get the left operand
    ASTNodePtr expr = this->logicAnd();

    // While the current token is 'or', parse the right operand
    while (this->match({Token::Type::Or})) {
        Token operatorToken = this->previous();
        ASTNodePtr right = this->logicAnd();

        // Create a logic expression node
        expr = std::make_unique<LogicExpr>(operatorToken, std::move(expr),
                                           std::move(right));
    }

    return expr;
}

ASTNodePtr Parser::logicAnd() {
    // Get the left operand
    ASTNodePtr expr = this->equality();

    // While the current token is 'and', parse the right operand
    while (this->match({Token::Type::And})) {
        Token operatorToken = this->previous();
        ASTNodePtr right = this->equality();

        // Create a logic expression node
        expr = std::make_unique<LogicExpr>(operatorToken, std::move(expr),
                                           std::move(right));
    }

    return expr;
}

ASTNodePtr Parser::equality() {
    // Get the left operand
    ASTNodePtr expr = this->comparison();

    // While the current token is a '==' or '!=', parse the right operand
    while (this->match({Token::Type::EqualEqual, Token::Type::BangEqual})) {
        Token operatorToken = this->previous();
        ASTNodePtr right = this->comparison();

        // Create a binary expression node
        expr = std::make_unique<BinaryExpr>(operatorToken, std::move(expr),
                                            std::move(right));
    }

    return expr;
}

ASTNodePtr Parser::comparison() {
    // Get the left operand
    ASTNodePtr expr = this->term();

    // While the current token is a comparison operator, parse the right operand
    while (this->match({Token::Type::Greater, Token::Type::GreaterEqual,
                        Token::Type::Less, Token::Type::LessEqual})) {
        Token operatorToken = this->previous();
        ASTNodePtr right = this->term();

        // Create a binary expression node
        expr = std::make_unique<BinaryExpr>(operatorToken, std::move(expr),
                                            std::move(right));
    }

    return expr;
}

ASTNodePtr Parser::term() {
    // Get the left operand
    ASTNodePtr expr = this->factor();

    // While the current token is a '+' or '-', parse the right operand
    while (this->match({Token::Type::Plus, Token::Type::Minus})) {
        Token operatorToken = this->previous();
        ASTNodePtr right = this->factor();

        // Create a binary expression node
        expr = std::make_unique<BinaryExpr>(operatorToken, std::move(expr),
                                            std::move(right));
    }

    return expr;
}

ASTNodePtr Parser::factor() {
    // Get the left operand
    ASTNodePtr expr = this->unary();

    // While the current token is a '*' or '/', parse the right operand
    while (this->match({Token::Type::Star, Token::Type::Slash})) {
        Token operatorToken = this->previous();
        ASTNodePtr right = this->unary();

        // Create a binary expression node
        expr = std::make_unique<BinaryExpr>(operatorToken, std::move(expr),
                                            std::move(right));
    }

    return expr;
}

ASTNodePtr Parser::unary() {
    // Check for a unary operator
    if (this->match({Token::Type::Minus, Token::Type::Not})) {
        Token operatorToken = this->previous();
        ASTNodePtr right = this->unary();
        return std::make_unique<UnaryExpr>(operatorToken, std::move(right));
    }

    // If no unary operator, parse a call expression
    return this->call();
}

ASTNodePtr Parser::call() {
    ASTNodePtr expr = this->primary();

    while (true) {
        if (this->match({Token::LeftParen}))
            expr = this->finishCall(std::move(expr));
        else
            break;
    };

    return expr;
}

ASTNodePtr Parser::finishCall(ASTNodePtr expr) {
    std::vector<ASTNodePtr> arguments;

    if (!this->check(Token::RightParen)) {
        do {
            arguments.push_back(this->expression());
        } while (this->match({Token::Comma}));
    }

    auto token =
        this->consume(Token::RightParen, "Expected ')' after arguments.");

    return std::make_unique<CallExpr>(token, std::move(expr),
                                      std::move(arguments));
}

ASTNodePtr Parser::primary() {
    // Make a literal node for number, string, true, false
    if (this->match({Token::Type::Number, Token::Type::String,
                     Token::Type::True, Token::Type::False})) {
        return std::make_unique<LiteralNode>(this->previous());
    }

    // Make a variable node for identifiers
    if (this->match({Token::Type::Identifier})) {
        return std::make_unique<VariableNode>(this->previous(),
                                              this->previous().lexeme);
    }

    // Try to parse a function definition
    const auto current_pos = this->current;
    try {
        if (this->match({Token::Type::LeftParen})) {
            this->current = current_pos; // Backtrack
            return this->function();
        }
    } catch (const ParserError &) {
        this->current = current_pos; // Backtrack
    }

    // If left parenthesis, parse a grouped expression
    if (this->match({Token::Type::LeftParen})) {
        ASTNodePtr expr = this->expression();
        this->consume(Token::Type::RightParen,
                      "Expected ')' after expression.");
        return expr;
    }

    // Else, throw an error for unexpected token
    throw ParserError(this->peek(), "Expected expression.");
}

ASTNodePtr Parser::function() {
    // Parse parameter list
    this->consume(Token::Type::LeftParen, "Expected '(' at start of function.");

    std::vector<ASTNodePtr> arguments;
    if (!this->check(Token::Type::RightParen)) {
        do {
            // Parse parameter type
            ASTNodePtr param_type = this->typeExpr();

            // Parse parameter name
            Token param_name = this->consume(Token::Type::Identifier,
                                             "Expected parameter name.");

            // Create variable node for parameter
            arguments.push_back(std::make_unique<VarDeclStmt>(
                std::move(param_type), param_name, nullptr, true));

        } while (this->match({Token::Type::Comma}));
    }

    this->consume(Token::Type::RightParen,
                  "Expected ')' after function parameters.");

    // Parse return type
    this->consume(Token::Type::Arrow,
                  "Expected '->' after function parameters.");
    ASTNodePtr return_type =
        this->match({Token::Type::None})
            ? std::make_unique<PrimitiveTypeNode>(this->previous())
            : this->typeExpr();

    // Parse function body
    this->consume(Token::Type::LeftBrace,
                  "Expected '{' at start of function body.");
    ASTNodePtr body = this->block();

    return std::make_unique<FunctionNode>(
        this->previous(), std::move(arguments), std::move(return_type),
        std::move(body));
}

/// Public interface

Parser::Parser(const std::vector<Token> &tokens) : current(0), tokens(tokens) {}

ASTNodePtr Parser::parse() {
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
