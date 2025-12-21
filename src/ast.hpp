#pragma once

#include "chunk.hpp"
#include "scanner.hpp"
#include "types.hpp"
#include "value.hpp"
#include <memory>

enum class ASTNodeType {
    Literal,
    UnaryExpression,
    BinaryExpression,
};

struct CompileContext {
    Chunk &chunk;
    TypeRegistry &typeRegistry;
};

struct CompileResult {
    TypeID result_type;
};

struct ASTNode {
    const ASTNodeType type;
    const Token token;

    ASTNode(ASTNodeType type, const Token &token) : type(type), token(token) {};
    virtual ~ASTNode() = default;
    virtual CompileResult compile(CompileContext &ctx) = 0;
    virtual void print(int indent = 0) = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

struct LiteralNode : public ASTNode {
    TypedValue value;

    LiteralNode(const Token &token);
    CompileResult compile(CompileContext &ctx) override;
    void print(int indent = 0) override;

  private:
    void print_object();
};

struct UnaryExpressionNode : public ASTNode {
    ASTNodePtr operand;

    UnaryExpressionNode(const Token &token, ASTNodePtr operand);
    CompileResult compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct BinaryExpressionNode : public ASTNode {
    ASTNodePtr left, right;

    BinaryExpressionNode(const Token &token, ASTNodePtr left, ASTNodePtr right);
    CompileResult compile(CompileContext &ctx) override;
    void print(int indent = 0) override;

  private:
    CompileResult compileArithmetic(CompileContext &ctx,
                                    const CompileResult &lresult,
                                    const CompileResult &rresult);
    CompileResult compileLogical(CompileContext &ctx,
                                  const CompileResult &lresult,
                                  const CompileResult &rresult);
};

Chunk compileAST(ASTNode *root);

