#pragma once

// Expressions are AST nodes that produce values.
// They can be evaluated to yield a result.
// They may have side effects.

#include "ast_core.hpp"

struct CallExpr : public ASTNode {
    ASTNodePtr callee;
    std::vector<ASTNodePtr> arguments;

    CallExpr(const Token &token, ASTNodePtr callee,
             std::vector<ASTNodePtr> arguments);
    AST_OVERRIDES;
};

struct AssignExpr : public ASTNode {
    ASTNodePtr target;
    ASTNodePtr value;

    AssignExpr(const Token &token, ASTNodePtr target, ASTNodePtr value);
    AST_OVERRIDES;

  private:
    void compileLocal(CompileContext &ctx, EntryID local_entry, int reg);
    void compileUpvalue(CompileContext &ctx, int reg);
};
