#include "parser.hpp"
#include "ast.hpp"
#include <array>
#include <format>
#include <iostream>
#include <iterator>
#include <memory>
#include "error.hpp"

struct ParseRule {
    // ParseFN is a pointer to a member function of Parser that returns
    // a unique_ptr<ASTNode> and takes no parameters.
    using ParseFn = std::unique_ptr<ASTNode> (Parser::*)();

    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

#define P(x) &Parser::x

constexpr std::array<ParseRule, 22> PARSE_RULES = {{
    { P(grouping), nullptr,   Precedence::None },     // LeftParen
    { nullptr,     nullptr,   Precedence::None },     // RightParen
    { P(unary),    P(binary), Precedence::Term },     // Minus
    { P(unary),    P(binary), Precedence::Term },     // Plus
    { nullptr,     P(binary), Precedence::Factor },   // Slash
    { nullptr,     P(binary), Precedence::Factor },   // Star
    { P(unary),    nullptr,   Precedence::Unary },    // Bang
    { nullptr,     nullptr,   Precedence::None },     // Identifier
    { P(literal),  nullptr,   Precedence::None },     // String
    { P(literal),  nullptr,   Precedence::None },     // Number
    { nullptr,     P(binary), Precedence::And },      // And 
    { nullptr,     P(binary), Precedence::Or },       // Or 
    { P(unary),    nullptr,   Precedence::Unary },    // Not 
    { P(literal),  nullptr,   Precedence::None },     // True
    { P(literal),  nullptr,   Precedence::None },     // False
    { nullptr,     nullptr,   Precedence::None },     // Error
    { nullptr,     nullptr,   Precedence::None }      // Eof
}};

#undef P

constexpr const ParseRule &getRule(Token::Type type) {
    return PARSE_RULES[static_cast<size_t>(type)];
}

void Parser::advance() {
    if (this->current != this->previous) {
        this->previous = this->current;
        this->current = std::next(this->current);
    } else
        this->previous = this->current + 1;

    if (this->current->type == Token::Type::Error)
        throw ParserError(*this->current, this->current->lexeme.c_str());
}

void Parser::consume(Token::Type type, const std::string &message) {
    if (current->type == type) {
        advance();
        return;
    }

    throw ParserError(*current, message.c_str());
}

std::unique_ptr<ASTNode> Parser::parsePrecedence(Precedence precedence) {
    // Advance to get the next token
    this->advance();

    // Execute the prefix parse function
    const ParseRule &prefixRule = getRule(this->previous->type);
    if (prefixRule.prefix == nullptr)
        throw ParserError(*this->previous, "Expected expression.");
    this->current_prefix = (this->*prefixRule.prefix)();

    // While the current token's precedence is >= the given precedence
    while (precedence <= getRule(this->current->type).precedence) {
        // Advance to get the next token
        this->advance();

        // Execute the infix parse function
        const ParseRule &infixRule = getRule(this->previous->type);
        if (infixRule.infix == nullptr)
            throw ParserError(*this->previous, "Expected expression.");
        this->current_prefix = (this->*infixRule.infix)();
    }

    return std::move(this->current_prefix);
}

std::unique_ptr<ASTNode> Parser::expression() {
    return parsePrecedence(Precedence::Assignment);
}

std::unique_ptr<ASTNode> Parser::unary() {
    // Get the operator token (already consumed)
    Token operatorToken = *this->previous;

    // Compile the operand
    auto operand = parsePrecedence(Precedence::Unary);

    // If the operand is +, just return the operand (unary plus is a no-op)
    if (operatorToken.type == Token::Type::Plus) {
        return operand;
    } else {
        // Make the unary expression node
        return std::make_unique<UnaryExpressionNode>(operatorToken,
                                                     std::move(operand));
    }
}

std::unique_ptr<ASTNode> Parser::binary() {
    // Get the operator token (already consumed)
    Token operatorToken = *this->previous;

    // Get the left operand from the current_prefix
    auto left = std::move(this->current_prefix);

    // Get the rule for this operator to determine its precedence
    const ParseRule &rule = getRule(operatorToken.type);
    Precedence precedence = rule.precedence;

    // Compile the right operand
    auto right = parsePrecedence(
        static_cast<Precedence>(static_cast<int>(precedence) + 1));

    // Make the binary expression node
    return std::make_unique<BinaryExpressionNode>(
        operatorToken, std::move(left), std::move(right));
}

std::unique_ptr<ASTNode> Parser::literal() {
    return std::make_unique<LiteralNode>(*this->previous);
}

std::unique_ptr<ASTNode> Parser::grouping() {
    // Compile the expression inside the parentheses
    auto expr = expression();

    // Consume the closing parenthesis
    consume(Token::Type::RightParen, "Expected ')' after expression.");

    return expr;
}

std::unique_ptr<ASTNode> Parser::parse(const std::vector<Token> &tokens) {
    // Prepare
    this->current = tokens.begin();
    this->previous = tokens.begin();

    try {
        // prime the parser
        advance();

        // for now, just parse a single expression
        auto ast = expression();

        // consume the EOF token
        consume(Token::Type::Eof, "Expected end of input.");

        return ast;

    } catch (const ParserError &error) {
        // return nullptr on error
        std::cerr << error.what();
        return nullptr;
    }
}

