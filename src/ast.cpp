#include "ast.hpp"
#include "chunk.hpp"

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

Chunk compileAST(ASTNode *root) {
    // Create compile context
    Chunk chunk;
    CompileContext ctx{chunk};

    // Compile the AST
    root->compile(ctx);

    return chunk;
}

