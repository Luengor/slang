#pragma once

#include "chunk.hpp"
#include "scanner.hpp"
#include "value.hpp"
#include <memory>

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
    ValueType resultType;

    ASTNode(ASTNodeType type, const Token &token) : type(type), token(token) {};
    virtual ~ASTNode() = default;
    virtual void compile(CompileContext &ctx) = 0;
    virtual void print(int indent = 0) = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

struct LiteralNode : public ASTNode {
    TypedValue value;

    LiteralNode(const Token &token);
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct UnaryExpressionNode : public ASTNode {
    ASTNodePtr operand;

    UnaryExpressionNode(const Token &token, ASTNodePtr operand);
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct BinaryExpressionNode : public ASTNode {
    ASTNodePtr left, right;

    BinaryExpressionNode(const Token &token, ASTNodePtr left, ASTNodePtr right);
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

Chunk compileAST(ASTNode *root);

