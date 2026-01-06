#pragma once

#include "ast_core.hpp"

struct PrimitiveTypeNode : public ASTNode {
    PrimitiveTypeNode(const Token &token);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

struct FunctionTypeNode : public ASTNode {
    std::vector<ASTNodePtr> param_types;
    ASTNodePtr return_type;

    FunctionTypeNode(const Token &token,
                     std::vector<ASTNodePtr> param_types,
                     ASTNodePtr return_type);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx, int reg = -1) override;
    void print(int indent = 0) override;
};

