#include "ast.hpp"
#include "chunk.hpp"
#include <iostream>

// LiteralNode Implementation
LiteralNode::LiteralNode(const Token &token)
    : ASTNode(ASTNodeType::Literal, token) {
    const double number = std::stod(token.lexeme);
    this->literal_type = Type::Number;
    this->value.number_value = number;
}

void LiteralNode::compile(CompileContext &ctx) {
    switch (this->literal_type) {
        case Type::Number:
            {
                const auto constant =
                    ctx.chunk.addConstant(this->value.number_value);
                ctx.chunk.write(OpCode::Constant, this->token.line);
                ctx.chunk.write(static_cast<uint8_t>(constant), this->token.line);
            }
    }
}

void LiteralNode::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    switch (this->literal_type) {
        case Type::Number:
            std::cout << "Literal(" << this->value.number_value << ")\n";
            break;
    }
}

// UnaryExpressionNode Implementation
UnaryExpressionNode::UnaryExpressionNode(const Token &token, ASTNodePtr operand)
    : ASTNode(ASTNodeType::UnaryExpression, token), operand(std::move(operand)) {}

void UnaryExpressionNode::compile(CompileContext &ctx) {
    operand->compile(ctx);

    switch (this->token.type) {
        case Token::Type::Minus:
            ctx.chunk.write(OpCode::Negate, this->token.line);
            break;

        default:
            // Handle error: unsupported unary operator
            break;
    }
}

void UnaryExpressionNode::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "UnaryExpression(" << this->token.lexeme << ")\n";
    this->operand->print(indent + 1);
}

// BinaryExpressionNode Implementation
BinaryExpressionNode::BinaryExpressionNode(const Token &token, ASTNodePtr left,
                                           ASTNodePtr right)
    : ASTNode(ASTNodeType::BinaryExpression, token), left(std::move(left)),
      right(std::move(right)) {}

void BinaryExpressionNode::compile(CompileContext &ctx) {
    left->compile(ctx);
    right->compile(ctx);

    switch (this->token.type) {
        case Token::Type::Plus:
            ctx.chunk.write(OpCode::Add, this->token.line);
            break;
        case Token::Type::Minus: 
            ctx.chunk.write(OpCode::Subtract, this->token.line);
            break;
        case Token::Type::Star: 
            ctx.chunk.write(OpCode::Multiply, this->token.line);
            break;
        case Token::Type::Slash: 
            ctx.chunk.write(OpCode::Divide, this->token.line);
            break;

        default:
            // Handle error: unsupported binary operator
            break;
    }
}

void BinaryExpressionNode::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "BinaryExpression(" << this->token.lexeme << ")\n";
    this->left->print(indent + 1);
    this->right->print(indent + 1);
}

Chunk compileAST(ASTNode *root) {
    // Create compile context
    Chunk chunk;
    CompileContext ctx{chunk};

    // Compile the AST
    root->compile(ctx);

    chunk.write(OpCode::Return, 0);

    return chunk;
}

