#pragma once

#include "ast_core.hpp"

struct PrimitiveTypeNode : public ASTNode {
    PrimitiveTypeNode(const Token &token);
    AST_OVERRIDES;
};

struct AliasTypeNode : public ASTNode {
    std::string alias_name;

    AliasTypeNode(const Token &token, const std::string &alias_name);
    AST_OVERRIDES;
};

struct FunctionTypeNode : public ASTNode {
    std::vector<ASTNodePtr> param_types;
    ASTNodePtr return_type;

    FunctionTypeNode(const Token &token, std::vector<ASTNodePtr> param_types,
                     ASTNodePtr return_type);
    AST_OVERRIDES;
};
