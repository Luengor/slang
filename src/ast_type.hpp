#pragma once

#include "ast_core.hpp"

struct PrimitiveTypeNode : public ASTNode {
    PrimitiveTypeNode(const Token &token);
    AST_OVERRIDES;
};

struct NamedTypeNode : public ASTNode {
    NamedTypeNode(const Token &token);
    AST_OVERRIDES;
};

struct FunctionTypeNode : public ASTNode {
    std::vector<ASTNodePtr> param_types;
    ASTNodePtr return_type;

    FunctionTypeNode(const Token &token, std::vector<ASTNodePtr> param_types,
                     ASTNodePtr return_type);
    AST_OVERRIDES;
};
