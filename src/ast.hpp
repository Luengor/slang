#pragma once

#include "chunk.hpp"
#include "scanner.hpp"
#include "types.hpp"
#include "value.hpp"
#include <memory>
#include <optional>

enum class ASTNodeType {
    Literal,
    UnaryExpression,
    CastExpression,
    BinaryExpression,
    LogicExpression,
};

struct CompileContext {
    Chunk &chunk;
    TypeRegistry &typeRegistry;
};

struct ASTNode {
    const ASTNodeType type;
    const Token token;
    std::optional<TypeID> result_type = std::nullopt;

    ASTNode(ASTNodeType type, const Token &token) : type(type), token(token) {};
    virtual ~ASTNode() = default;
    virtual void resolveType(CompileContext &ctx) = 0;
    virtual void compile(CompileContext &ctx) = 0;
    virtual void print(int indent = 0) = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

struct LiteralNode : public ASTNode {
    TypedValue value;

    LiteralNode(const Token &token);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;

  private:
    void print_object();
};

struct UnaryExpr : public ASTNode {
    ASTNodePtr operand;

    UnaryExpr(const Token &token, ASTNodePtr operand);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct CastExpr : public ASTNode {
    ASTNodePtr operand;
    OpCode cast_op;

    CastExpr(const Token &token, ASTNodePtr operand, TypeID target_type);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct BinaryExpr : public ASTNode {
    ASTNodePtr left, right;

    BinaryExpr(const Token &token, ASTNodePtr left, ASTNodePtr right);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;

  private:
    void compileArithmetic(CompileContext &ctx);
    void compileEquality(CompileContext &ctx);
    void compileComparison(CompileContext &ctx);
};

struct LogicExpr : public ASTNode {
    ASTNodePtr left, right;

    LogicExpr(const Token &token, ASTNodePtr left, ASTNodePtr right);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

Chunk compileAST(ASTNode *root);

