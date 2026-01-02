#pragma once

#include "compile_context.hpp"
#include "scanner.hpp"
#include <memory>
#include <optional>

// AST Node Types 
enum class ASTNodeType {
    Literal,
    Function,
    Variable,
    UnaryExpr,
    CastExpr,
    BinaryExpr,
    TernaryExpr,
    LogicExpr,
    ExprStmt,
    BlockStmt,
    VarDeclStmt,
    AssignExpr,
    IfStmt,
    WhileStmt,
    PrimitiveType,
    FunctionType,
    CallExpr,
    ReturnStmt,
};

// Base class for all AST nodes
struct ASTNode {
    // The type of this AST node
    const ASTNodeType type;

    // "A" token associated with this node (for error reporting)
    const Token token;

    // The result type of this node or nullopt if not yet resolved
    // (for anything non-returning, it will be 'none' type)
    std::optional<TypeID> result_type = std::nullopt;

    ASTNode(ASTNodeType type, const Token &token);
    virtual ~ASTNode() = default;

    // Resolve the type of this AST node
    // After this is called, result_type should be set
    virtual void resolveType(CompileContext &ctx) = 0;

    // Compile this AST node into the given compile context
    virtual void compile(CompileContext &ctx) = 0;

    // Print the AST node for debugging
    virtual void print(int indent = 0) = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

struct FunctionObj;

std::unique_ptr<FunctionObj> compileAST(ASTNode *root);

