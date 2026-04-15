#pragma once

// Pure expressions are AST nodes that have no side effects.
// They can be evaluated without changing program state.

#include "ast_core.hpp"
#include "compile_context.hpp"

struct LiteralNode : public ASTNode {
    TypedValue value;

    LiteralNode(const Token &token);
    ~LiteralNode();
    AST_OVERRIDES;

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
    ~FunctionNode();
    AST_OVERRIDES;
};

struct NativeFunctionObj;

struct VariableNode : public ASTNode {
    std::string name;
    std::variant<EntryID, NativeFunctionObj *> resolution;

    VariableNode(const Token &token, const std::string &name);
    AST_OVERRIDES;

  private:
    void compileLocal(CompileContext &ctx, int reg);
    void compileUpvalue(CompileContext &ctx, int reg);
};

struct UnaryExpr : public ASTNode {
    ASTNodePtr operand;

    UnaryExpr(const Token &token, ASTNodePtr operand);
    AST_OVERRIDES;
};

struct CastExpr : public ASTNode {
    ASTNodePtr operand;
    OpCode cast_op;
    TypeID target_type;

    static std::optional<ASTNodePtr>
    tryCast(ASTNodePtr operand, TypeID target_type, CompileContext &ctx);

    static std::optional<std::pair<ASTNodePtr, ASTNodePtr>>
    tryCommonCast(ASTNodePtr left, ASTNodePtr right, CompileContext &ctx);

    CastExpr(const Token &token, ASTNodePtr operand, TypeID target_type);
    AST_OVERRIDES;
};

struct BinaryExpr : public ASTNode {
    ASTNodePtr left, right;

    BinaryExpr(const Token &token, ASTNodePtr left, ASTNodePtr right);
    AST_OVERRIDES;

  private:
    int left_r, right_r;

    void compileArithmetic(CompileContext &ctx);
    void compileEquality(CompileContext &ctx);
    void compileComparison(CompileContext &ctx);
};

struct TernaryExpr : public ASTNode {
    ASTNodePtr condition, then_branch, else_branch;

    TernaryExpr(const Token &token, ASTNodePtr condition,
                ASTNodePtr then_branch, ASTNodePtr else_branch);
    AST_OVERRIDES;
};

struct LogicExpr : public ASTNode {
    ASTNodePtr left, right;

    LogicExpr(const Token &token, ASTNodePtr left, ASTNodePtr right);
    AST_OVERRIDES;

  private:
    void compileAnd(CompileContext &ctx);
    void compileOr(CompileContext &ctx);
};

struct ArrayLiteralExpr : public ASTNode {
    std::vector<ASTNodePtr> elements;

    ArrayLiteralExpr(const Token &token, std::vector<ASTNodePtr> elements);
    AST_OVERRIDES;
};

struct IndexExpr : public ASTNode {
    ASTNodePtr array;
    ASTNodePtr index;

    IndexExpr(const Token &token, ASTNodePtr array, ASTNodePtr index);
    AST_OVERRIDES;
};
