#pragma once

// Statements are AST nodes that have side effects.
// (statements can, and often do, behave like expressions)
// ((but I have to draw the line somewhere))

#include "ast_core.hpp"

struct ExprStmt : public ASTNode {
    ASTNodePtr expression;

    ExprStmt(const Token &token, ASTNodePtr expression);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct BlockStmt : public ASTNode {
    std::vector<ASTNodePtr> statements;
    PopCount pop;

    BlockStmt(const Token &token, std::vector<ASTNodePtr> statements);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct VarDeclStmt : public ASTNode {
    ASTNodePtr type_expr;
    ASTNodePtr initializer;

    VarDeclStmt(ASTNodePtr type_expr, const Token &name_token,
                ASTNodePtr initializer);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct IfStmt : public ASTNode {
    ASTNodePtr condition;
    ASTNodePtr then_branch;
    ASTNodePtr else_branch;

    IfStmt(const Token &token, ASTNodePtr condition, ASTNodePtr then_branch,
           ASTNodePtr else_branch);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct WhileStmt : public ASTNode {
    ASTNodePtr condition;
    ASTNodePtr body;

    WhileStmt(const Token &token, ASTNodePtr condition, ASTNodePtr body);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct AssignExpr : public ASTNode {
    ASTNodePtr target;
    ASTNodePtr value;

    AssignExpr(const Token &token, ASTNodePtr target, ASTNodePtr value);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

