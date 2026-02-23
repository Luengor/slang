#pragma once

// Statements are AST nodes that do not produce values.
// As such, passing a register to compile them is an error.

#include "ast_core.hpp"
#include "compile_context.hpp"

struct ExprStmt : public ASTNode {
    ASTNodePtr expression;

    ExprStmt(const Token &token, ASTNodePtr expression);
    AST_OVERRIDES;
};

struct BlockStmt : public ASTNode {
    std::vector<ASTNodePtr> statements;
    bool has_end_return = false;

    BlockStmt(const Token &token, std::vector<ASTNodePtr> statements);
    AST_OVERRIDES;
};

struct VarDeclStmt : public ASTNode {
    ASTNodePtr type_expr;
    ASTNodePtr initializer;
    EntryID entry_id;
    bool is_in_function_definition;

    VarDeclStmt(ASTNodePtr type_expr, const Token &name_token,
                ASTNodePtr initializer, bool is_in_function_definition = false);
    AST_OVERRIDES;
};

struct IfStmt : public ASTNode {
    ASTNodePtr condition;
    ASTNodePtr then_branch;
    ASTNodePtr else_branch;

    IfStmt(const Token &token, ASTNodePtr condition, ASTNodePtr then_branch,
           ASTNodePtr else_branch);
    AST_OVERRIDES;
};

struct WhileStmt : public ASTNode {
    ASTNodePtr condition;
    ASTNodePtr body;

    WhileStmt(const Token &token, ASTNodePtr condition, ASTNodePtr body);
    AST_OVERRIDES;
};

struct ReturnStmt : public ASTNode {
    ASTNodePtr return_expr;

    ReturnStmt(const Token &token, ASTNodePtr return_expr);
    AST_OVERRIDES;
};

