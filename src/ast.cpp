#include "ast.hpp"
#include "chunk.hpp"

// ASTNode Implementation
ASTNode::~ASTNode() {
    for (ASTNode *child : this->children) {
        delete child;
    }
}

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
UnaryExpressionNode::UnaryExpressionNode(const Token &token, ASTNode *operand)
    : ASTNode(ASTNodeType::UnaryExpression, token) {
    this->children.push_back(operand);
    this->op = Operator::Negation;
}

void UnaryExpressionNode::compile(CompileContext &ctx) {
    ASTNode *operand = this->children[0];
    operand->compile(ctx);

    switch (this->op) {
        case Operator::Negation:
            ctx.chunk.write(OpCode::Negate, this->token.line);
            break;
    }
}

// BinaryExpressionNode Implementation
BinaryExpressionNode::BinaryExpressionNode(const Token &token, ASTNode *left,
                                           ASTNode *right)
    : ASTNode(ASTNodeType::BinaryExpression, token) {
    this->children.push_back(left);
    this->children.push_back(right);
}

void BinaryExpressionNode::compile(CompileContext &ctx) {
    ASTNode *left = this->children[0];
    ASTNode *right = this->children[1];

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

