#pragma once

#include "chunk.hpp"
#include "scanner.hpp"
#include <vector>

enum class ASTNodeType {
    Literal,
    UnaryExpression,
    BinaryExpression,
};

struct ASTNode {
    const ASTNodeType type;
    const Token token;

    std::vector<ASTNode *> children;

    ASTNode(ASTNodeType type, const Token &token) : type(type), token(token) {};
    virtual ~ASTNode();
    virtual void compile(Chunk &chunk) = 0;
};

struct LiteralNode : public ASTNode {
    enum Type {
        Number,
    } literal_type;

    union {
        double number_value;
    } value;

    LiteralNode(const Token &token);
    void compile(Chunk &chunk) override;
};

struct UnaryExpressionNode : public ASTNode {
    enum Operator {
        Negation,
    } op;

    UnaryExpressionNode(const Token &token, ASTNode *operand);
    void compile(Chunk &chunk) override;
};

struct BinaryExpressionNode : public ASTNode {
    BinaryExpressionNode(const Token &token, ASTNode *left, ASTNode *right);
    void compile(Chunk &chunk) override;
};
