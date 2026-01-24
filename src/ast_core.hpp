#pragma once

#include "compile_context.hpp"
#include "scanner.hpp"
#include <memory>

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

struct ResultInfo {
    TypeID type;
    bool is_var = false;
    int reg = -1;
};

// Base class for all AST nodes
struct ASTNode {
    // The type of this AST node
    const ASTNodeType type;

    // "A" token associated with this node (for error reporting)
    const Token token;

    // The result of this node
    ResultInfo result;

    ASTNode(ASTNodeType type, const Token &token);
    virtual ~ASTNode() = default;

    bool type_resolved = false;
    virtual void resolveType(CompileContext &ctx) = 0;

    bool has_compiled = false;
    // Compile this AST node into the given compile context
    virtual void compile(CompileContext &ctx, int reg = -1) = 0;

    // Print the AST node for debugging
    virtual void print(int indent = 0) = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

#define AST_OVERRIDES                     \
    void resolveType(CompileContext &ctx) override; \
    void compile(CompileContext &ctx, int reg = -1) override; \
    void print(int indent = 0) override


struct FunctionObj;

std::unique_ptr<FunctionObj> compileAST(ASTNode *root);

