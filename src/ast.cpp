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

void LiteralNode::compile(Chunk &chunk) {
    switch (this->literal_type) {
        case Type::Number:
            {
                const auto constant =
                    chunk.addConstant(this->value.number_value);
                chunk.write(OpCode::Constant, this->token.line);
                chunk.write(static_cast<uint8_t>(constant), this->token.line);
            }
    }
}

// UnaryExpressionNode Implementation
UnaryExpressionNode::UnaryExpressionNode(const Token &token, ASTNode *operand)
    : ASTNode(ASTNodeType::UnaryExpression, token) {
    this->children.push_back(operand);
    this->op = Operator::Negation;
}

void UnaryExpressionNode::compile(Chunk &chunk) {
    ASTNode *operand = this->children[0];
    operand->compile(chunk);

    switch (this->op) {
        case Operator::Negation:
            chunk.write(OpCode::Negate, this->token.line);
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

void BinaryExpressionNode::compile(Chunk &chunk) {
    ASTNode *left = this->children[0];
    ASTNode *right = this->children[1];

    left->compile(chunk);
    right->compile(chunk);

    switch (this->token.type) {
        case Token::Type::Plus:
            chunk.write(OpCode::Add, this->token.line);
            break;
        case Token::Type::Minus: 
            chunk.write(OpCode::Subtract, this->token.line);
            break;
        case Token::Type::Star: 
            chunk.write(OpCode::Multiply, this->token.line);
            break;
        case Token::Type::Slash: 
            chunk.write(OpCode::Divide, this->token.line);
            break;

        default:
            // Handle error: unsupported binary operator
            break;
    }
}

