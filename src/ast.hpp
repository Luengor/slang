#pragma once

#include "chunk.hpp"
#include "scanner.hpp"
#include "types.hpp"
#include "value.hpp"
#include <memory>
#include <optional>
#include <vector>

enum class ASTNodeType {
    Literal,
    Variable,
    UnaryExpr,
    CastExpr,
    BinaryExpr,
    LogicExpr,
    ExprStmt,
    BlockStmt,
    VarDeclStmt,
    AssignExpr,
    IfStmt,
    WhileStmt,
};

struct Local {
    std::string name;
    TypeID type;
    int depth;
};

struct CompileContext {
    Chunk &chunk;
    TypeRegistry &typeRegistry;

    int scope_depth = 0;
    std::vector<Local> locals = {};

    int addLocal(const std::string &name, TypeID type);
    int findLocal(const std::string &name);
    void enterScope();
    int exitScope();
};

struct ASTNode {
    const ASTNodeType type;
    const Token token;
    std::optional<TypeID> result_type = std::nullopt;

    ASTNode(ASTNodeType type, const Token &token) : type(type), token(token) {};
    virtual ~ASTNode() = default;
    virtual void resolveType(CompileContext &ctx) = 0;
    virtual void compile(CompileContext &ctx) = 0;
    virtual void print(int indent = 0) = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

struct LiteralNode : public ASTNode {
    TypedValue value;

    LiteralNode(const Token &token);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;

  private:
    void print_object();
};

struct VariableNode : public ASTNode {
    std::string name;
    int local_index = -1;

    VariableNode(const Token &token, const std::string &name);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct UnaryExpr : public ASTNode {
    ASTNodePtr operand;

    UnaryExpr(const Token &token, ASTNodePtr operand);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct CastExpr : public ASTNode {
    ASTNodePtr operand;
    OpCode cast_op;
    TypeID target_type;

    CastExpr(const Token &token, ASTNodePtr operand, TypeID target_type);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct BinaryExpr : public ASTNode {
    ASTNodePtr left, right;

    BinaryExpr(const Token &token, ASTNodePtr left, ASTNodePtr right);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;

  private:
    void compileArithmetic(CompileContext &ctx);
    void compileEquality(CompileContext &ctx);
    void compileComparison(CompileContext &ctx);
};

struct LogicExpr : public ASTNode {
    ASTNodePtr left, right;

    LogicExpr(const Token &token, ASTNodePtr left, ASTNodePtr right);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;

private:
    void compileAnd(CompileContext &ctx);
    void compileOr(CompileContext &ctx);
};

struct ExprStmt : public ASTNode {
    ASTNodePtr expression;

    ExprStmt(const Token &token, ASTNodePtr expression);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct BlockStmt : public ASTNode {
    std::vector<ASTNodePtr> statements;
    int pop = 0;

    BlockStmt(const Token &token, std::vector<ASTNodePtr> statements);
    void resolveType(CompileContext &ctx) override;
    void compile(CompileContext &ctx) override;
    void print(int indent = 0) override;
};

struct VarDeclStmt : public ASTNode {
    ASTNodePtr initializer;
    Token type_token;

    VarDeclStmt(const Token &type_token, const Token &name_token,
                ASTNodePtr initializer);
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

Chunk compileAST(ASTNode *root);

