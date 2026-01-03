#pragma once

// Expressions are AST nodes that are not statements.
// Statements are AST nodes that have side effects.
// (statements can, and often do, behave like expressions)
// ((but I have to draw the line somewhere))

#include "ast_core.hpp"
#include "compile_context.hpp"

struct LiteralNode : public ASTNode {
    TypedValue value;

    LiteralNode(const Token &token);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;

  private:
    void print_object();
};

struct FunctionNode : public ASTNode {
    std::vector<ASTNodePtr> arguments;
    ASTNodePtr return_type;
    ASTNodePtr body;
    std::unique_ptr<CompileContext> fn_ctx;

    FunctionNode(const Token &token, std::vector<ASTNodePtr> arguments,
                 ASTNodePtr return_type, ASTNodePtr body);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

struct NativeFunctionObj;

struct VariableNode : public ASTNode {
    std::string name;
    std::variant<EntryID, NativeFunctionObj *> resolution;

    VariableNode(const Token &token, const std::string &name);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

struct UnaryExpr : public ASTNode {
    ASTNodePtr operand;

    UnaryExpr(const Token &token, ASTNodePtr operand);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

struct CastExpr : public ASTNode {
    ASTNodePtr operand;
    OpCode cast_op;
    TypeID target_type;

    static std::optional<ASTNodePtr>
    tryCast(ASTNodePtr operand, TypeID target_type, CompileContext &ctx);

    static std::optional<std::pair<ASTNodePtr, ASTNodePtr>>
    tryCommonCast(ASTNodePtr left, ASTNodePtr right,
                  CompileContext &ctx);

    CastExpr(const Token &token, ASTNodePtr operand, TypeID target_type);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

struct BinaryExpr : public ASTNode {
    ASTNodePtr left, right;

    BinaryExpr(const Token &token, ASTNodePtr left, ASTNodePtr right);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;

  private:
    void compileArithmetic(CompileContext &ctx);
    void compileEquality(CompileContext &ctx);
    void compileComparison(CompileContext &ctx);
};

struct TernaryExpr : public ASTNode {
    ASTNodePtr condition, then_branch, else_branch;

    TernaryExpr(const Token &token, ASTNodePtr condition,
                ASTNodePtr then_branch, ASTNodePtr else_branch);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

struct LogicExpr : public ASTNode {
    ASTNodePtr left, right;

    LogicExpr(const Token &token, ASTNodePtr left, ASTNodePtr right);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;

private:
    void compileAnd(CompileContext &ctx);
    void compileOr(CompileContext &ctx);
};

struct CallExpr : public ASTNode {
    ASTNodePtr callee;
    std::vector<ASTNodePtr> arguments;

    CallExpr(const Token &token, ASTNodePtr callee,
             std::vector<ASTNodePtr> arguments);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

