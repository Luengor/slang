#include "ast_type.hpp"
#include "ast_macros.hpp"
#include "ast_stmt.hpp"
#include "error.hpp"
#include <iostream>

PrimitiveTypeNode::PrimitiveTypeNode(const Token &token)
    : ASTNode(ASTNodeType::PrimitiveType, token) {}

void PrimitiveTypeNode::resolveType(CompileContext &ctx) {
    TypeGuard;

    // Determine the primitive type based on the token
    PrimitiveKind kind;
    switch (this->token.type) {
        case Token::Type::Fixed:
            kind = PrimitiveKind::Fixed;
            break;
        case Token::Type::Float:
            kind = PrimitiveKind::Floating;
            break;
        case Token::Type::Bool:
            kind = PrimitiveKind::Boolean;
            break;
        case Token::Type::Str:
            kind = PrimitiveKind::String;
            break;
        case Token::Type::None:
            kind = PrimitiveKind::None;
            break;
        default:
            throw ParserError(this->token,
                              "Invalid token for PrimitiveTypeNode.");
    }

    type(this) = ctx.typeRegistry.getPrimitive(kind);
}

void PrimitiveTypeNode::compile(CompileContext &, int) {
    throw ParserError(this->token, "PrimitiveTypeNode should not be compiled.");
}

void PrimitiveTypeNode::print(int indent) {
    for (int i = 0; i < indent; i++)
        std::cout << "  ";
    std::cout << "PrimitiveTypeNode(" << this->token.lexeme << ")\n";
}

AliasTypeNode::AliasTypeNode(const Token &token, const std::string &alias_name)
    : ASTNode(ASTNodeType::AliasType, token), alias_name(alias_name) {}

void AliasTypeNode::resolveType(CompileContext &ctx) {
    TypeGuard;

    auto alias_entry = ctx.typeAliasTable.findAliasInScope(this->alias_name);
    if (!alias_entry.has_value()) {
        throw ParserError(this->token, "Undefined type alias: " + this->alias_name);
    }

    auto &entry = ctx.typeAliasTable.getEntry(alias_entry.value());

    // Resolve referenced alias lazily to support mutual recursion.
    if (entry.decl != nullptr) {
        entry.decl->resolveType(ctx);
    }

    type(this) = entry.alias_type_id;
}

void AliasTypeNode::compile(CompileContext &, int) {
    throw ParserError(this->token, "AliasTypeNode should not be compiled.");
}

void AliasTypeNode::print(int indent) {
    for (int i = 0; i < indent; i++)
        std::cout << "  ";
    std::cout << "AliasTypeNode(" << this->alias_name << ")\n";
}

FunctionTypeNode::FunctionTypeNode(const Token &token,
                                   std::vector<ASTNodePtr> param_types,
                                   ASTNodePtr return_type)
    : ASTNode(ASTNodeType::FunctionType, token),
      param_types(std::move(param_types)), return_type(std::move(return_type)) {
}

void FunctionTypeNode::resolveType(CompileContext &ctx) {
    TypeGuard;

    // Resolve parameter types
    std::vector<TypeID> param_type_ids;
    for (auto &param_type : this->param_types) {
        param_type->resolveType(ctx);
        param_type_ids.push_back(type(param_type));
    }

    // Resolve return type
    this->return_type->resolveType(ctx);
    TypeID return_type_id = type(this->return_type);

    // Get the function type ID
    type(this) = ctx.typeRegistry.getFunction(param_type_ids, return_type_id);
}

void FunctionTypeNode::compile(CompileContext &, int) {
    throw ParserError(this->token, "FunctionTypeNode should not be compiled.");
}

void FunctionTypeNode::print(int indent) {
    for (int i = 0; i < indent; i++)
        std::cout << "  ";
    std::cout << "FunctionTypeNode\n";
    for (const auto &param_type : this->param_types) {
        param_type->print(indent + 1);
    }
    this->return_type->print(indent + 1);
}
