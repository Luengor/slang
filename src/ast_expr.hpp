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
    AST_OVERRIDES;

  private:
    void print_object();
};

struct FunctionNode : public ASTNode {
    std::vector<ASTNodePtr> arguments;
    ASTNodePtr return_type;
    ASTNodePtr body;
    std::unique_ptr<CompileContext> fn_ctx;
    EntryID self_entry_id;

    FunctionNode(const Token &token, std::vector<ASTNodePtr> arguments,
                 ASTNodePtr return_type, ASTNodePtr body);
    AST_OVERRIDES;
};

struct NativeFunctionObj;

struct VariableNode : public ASTNode {
    std::string name;
    std::variant<EntryID, NativeFunctionObj *> resolution;

    VariableNode(const Token &token, const std::string &name);
    AST_OVERRIDES;
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
    tryCommonCast(ASTNodePtr left, ASTNodePtr right,
                  CompileContext &ctx);

    CastExpr(const Token &token, ASTNodePtr operand, TypeID target_type);
    AST_OVERRIDES;
};

struct BinaryExpr : public ASTNode {
    ASTNodePtr left, right;

    BinaryExpr(const Token &token, ASTNodePtr left, ASTNodePtr right);
    AST_OVERRIDES;

  private:
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

struct CallExpr : public ASTNode {
    ASTNodePtr callee;
    std::vector<ASTNodePtr> arguments;

    CallExpr(const Token &token, ASTNodePtr callee,
             std::vector<ASTNodePtr> arguments);
    AST_OVERRIDES;
};

