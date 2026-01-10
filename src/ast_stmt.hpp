#pragma once

// Statements are AST nodes that have side effects.
// (statements can, and often do, behave like expressions)
// ((but I have to draw the line somewhere))

#include "ast_core.hpp"
#include "compile_context.hpp"

struct ExprStmt : public ASTNode {
    ASTNodePtr expression;

    ExprStmt(const Token &token, ASTNodePtr expression);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

struct BlockStmt : public ASTNode {
    std::vector<ASTNodePtr> statements;

    BlockStmt(const Token &token, std::vector<ASTNodePtr> statements);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

struct VarDeclStmt : public ASTNode {
    ASTNodePtr type_expr;
    ASTNodePtr initializer;
    EntryID entry_id;
    bool is_in_function_definition;

    VarDeclStmt(ASTNodePtr type_expr, const Token &name_token,
                ASTNodePtr initializer, bool is_in_function_definition = false);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

struct IfStmt : public ASTNode {
    ASTNodePtr condition;
    ASTNodePtr then_branch;
    ASTNodePtr else_branch;

    IfStmt(const Token &token, ASTNodePtr condition, ASTNodePtr then_branch,
           ASTNodePtr else_branch);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

struct WhileStmt : public ASTNode {
    ASTNodePtr condition;
    ASTNodePtr body;

    WhileStmt(const Token &token, ASTNodePtr condition, ASTNodePtr body);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

struct AssignExpr : public ASTNode {
    ASTNodePtr target;
    ASTNodePtr value;

    AssignExpr(const Token &token, ASTNodePtr target, ASTNodePtr value);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

struct ReturnStmt : public ASTNode {
    ASTNodePtr return_expr;

    ReturnStmt(const Token &token, ASTNodePtr return_expr);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

