#pragma once

#include "chunk.hpp"
#include "scanner.hpp"
#include <vector>

enum class ASTNodeType {
    Literal,
    UnaryExpression,
    BinaryExpression,
};

struct CompileContext {
    Chunk &chunk;
};

struct ASTNode {
    const ASTNodeType type;
    const Token token;

    std::vector<ASTNode *> children;

    ASTNode(ASTNodeType type, const Token &token) : type(type), token(token) {};
    virtual ~ASTNode();
    virtual void compile(CompileContext &ctx) = 0;
};

struct LiteralNode : public ASTNode {
    enum Type {
        Number,
    } literal_type;

    union {
        double number_value;
    } value;

    LiteralNode(const Token &token);
    void compile(CompileContext &ctx) override;
};

struct UnaryExpressionNode : public ASTNode {
    enum Operator {
        Negation,
    } op;

    UnaryExpressionNode(const Token &token, ASTNode *operand);
    void compile(CompileContext &ctx) override;
};

struct BinaryExpressionNode : public ASTNode {
    BinaryExpressionNode(const Token &token, ASTNode *left, ASTNode *right);
    void compile(CompileContext &ctx) override;
};

Chunk compileAST(ASTNode *root);

