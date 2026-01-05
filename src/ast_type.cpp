#include "ast_type.hpp"
#include "error.hpp"
#include <iostream>
#include "ast_macros.hpp"

PrimitiveTypeNode::PrimitiveTypeNode(const Token &token)
    : ASTNode(ASTNodeType::PrimitiveType, token) {}

void PrimitiveTypeNode::resolveType(CompileContext &ctx) {
    ResolveGuard;

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
            throw ParserError(
                this->token,
                "Invalid token for PrimitiveTypeNode.");
    }

    type(this) = ctx.typeRegistry.getPrimitive(kind);
}

void PrimitiveTypeNode::compile(CompileContext &_, int reg) {
    throw ParserError(
        this->token,
        "PrimitiveTypeNode should not be compiled.");
}

void PrimitiveTypeNode::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "PrimitiveTypeNode(" << this->token.lexeme << ")\n";
}

FunctionTypeNode::FunctionTypeNode(
    const Token &token,
    std::vector<ASTNodePtr> param_types,
    ASTNodePtr return_type)
    : ASTNode(ASTNodeType::FunctionType, token),
      param_types(std::move(param_types)),
      return_type(std::move(return_type)) {}

void FunctionTypeNode::resolveType(CompileContext &ctx) {
    ResolveGuard;

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
    type(this) = ctx.typeRegistry.getFunction(
        param_type_ids, return_type_id);
}

void FunctionTypeNode::compile(CompileContext &_, int reg) {
    throw ParserError(
        this->token,
        "FunctionTypeNode should not be compiled.");
}

void FunctionTypeNode::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "FunctionTypeNode\n";
    for (const auto &param_type : this->param_types) {
        param_type->print(indent + 1);
    }
    this->return_type->print(indent + 1);
}

