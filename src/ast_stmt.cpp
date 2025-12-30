#include "ast_stmt.hpp"
#include "ast_expr.hpp"
#include "error.hpp"
#include <iostream>

#define ResolveGuard \
    if (this->result_type.has_value()) { \
        return; \
    }

// ExprStmt Implementation
ExprStmt::ExprStmt(const Token &token, ASTNodePtr expression)
    : ASTNode(ASTNodeType::ExprStmt, token),
      expression(std::move(expression)) {}

void ExprStmt::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // Resolve the expression type if any
    if (this->expression)
        this->expression->resolveType(ctx);

    // Expression statements have no result type
    this->result_type = ctx.typeRegistry.noneType();
}

void ExprStmt::compile(CompileContext &ctx) {
    // If there is no expression, nothing to compile
    if (!this->expression) {
        return;
    }

    // Compile the expression
    this->expression->compile(ctx);

    // Pop the result off the stack since it's not used
    ctx.chunk->write(OpCode::Pop, this->token.line);
}

void ExprStmt::print(int indent) {
    if (!this->expression) {
        for (int i = 0; i < indent; i++) std::cout << "  ";
        std::cout << "ExprStmt(Empty)\n";
        return;
    }

    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "ExprStmt\n";
    this->expression->print(indent + 1);
}

// BlockStmt Implementation

BlockStmt::BlockStmt(const Token &token, std::vector<ASTNodePtr> statements)
    : ASTNode(ASTNodeType::BlockStmt, token),
      statements(std::move(statements)) {}

void BlockStmt::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // Enter a new scope
    ctx.enterScope();

    // Resolve types for all statements
    for (auto &stmt : this->statements) {
        stmt->resolveType(ctx);
    }

    // Exit the scope
    this->pop = ctx.exitScope();

    // For now, block statements have no result type
    // Maybe in the future we can have the last statement's type?
    this->result_type = ctx.typeRegistry.noneType();
}

void BlockStmt::compile(CompileContext &ctx) {
    // Compile all statements in the block
    for (auto &stmt : this->statements) {
        stmt->compile(ctx);
    }

    // Pop local variables declared in this block
    for (int i = 0; i < this->pop; i++) {
        ctx.chunk->write(OpCode::Pop); // Use the line of the last statement
    }
}

void BlockStmt::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "BlockStmt\n";
    for (auto &stmt : this->statements) {
        stmt->print(indent + 1);
    }
}

// VarDeclStmt Implementation
VarDeclStmt::VarDeclStmt(ASTNodePtr type_expr, const Token &name_token,
                         ASTNodePtr initializer)
    : ASTNode(ASTNodeType::VarDeclStmt, name_token),
      type_expr(std::move(type_expr)), initializer(std::move(initializer)) {}

void VarDeclStmt::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // If we have a type expression, resolve it
    if (this->type_expr) {
        this->type_expr->resolveType(ctx);
    }

    // If there's an initializer, resolve its type
    if (this->initializer) {
        this->initializer->resolveType(ctx);
    }

    // If this is auto, infer the type from the initializer
    if (this->type_expr == nullptr) {
        if (this->initializer == nullptr) {
            throw ParserError(
                this->token,
                "Auto variable declaration requires an initializer.");
        }

        this->result_type = this->initializer->result_type;
    } else {
        // Otherwise, use the declared type
        this->result_type = this->type_expr->result_type;

        // If there is an initializer, ensure it matches the declared type
        if (this->initializer) {
            if (this->initializer->result_type != this->result_type) {
                // Try to insert a cast if possible
                auto castOp = ctx.typeRegistry.getCastOp(
                    this->initializer->result_type.value(), this->result_type.value());
                if (!castOp.has_value()) {
                    throw ParserError(
                        this->token,
                        "Incompatible types in variable initializer.");
                }
            }
        }
    }

    // Push the variable to the locals
    int local = ctx.addLocal(this->token.lexeme, this->result_type.value());
    if (local == -1) {
        throw ParserError(
            this->token,
            "Variable with the same name already declared in this scope.");
    }
}

void VarDeclStmt::compile(CompileContext &ctx) {
    // Compile the initializer if present
    if (this->initializer) {
        this->initializer->compile(ctx);
    } else {
        // Default initializer
        // for now, push false and good luck if it's not a boolean :)
        Value defaultValue {.boolean = false};
        const auto constant = ctx.chunk->addConstant(defaultValue);
        ctx.chunk->write(OpCode::Constant, this->token.line);
        ctx.chunk->write(static_cast<uint8_t>(constant));
    }

    // Nothing more to do, we pray now
}

void VarDeclStmt::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "VarDeclStmt(" << this->token.lexeme << " : "
              << this->result_type.value() << ")\n";
    if (this->initializer) {
        this->initializer->print(indent + 1);
    }
}

// AssignExpr Implementation
AssignExpr::AssignExpr(const Token &token, ASTNodePtr target,
                         ASTNodePtr value)
    : ASTNode(ASTNodeType::AssignExpr, token),
      target(std::move(target)), value(std::move(value)) {}

void AssignExpr::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // Resolve target and value types first
    this->target->resolveType(ctx);
    this->value->resolveType(ctx);

    // Ensure the value can be assigned to the target
    if (this->target->result_type != this->value->result_type) {
        // Try to insert a cast if possible
        auto castOp = ctx.typeRegistry.getCastOp(
            this->value->result_type.value(), this->target->result_type.value());

        if (!castOp.has_value()) {
            throw ParserError(
                this->token,
                "Incompatible types in assignment expression.");
        }

        this->value = std::make_unique<CastExpr>(
            this->token,
            std::move(this->value),
            this->target->result_type.value());
        this->value->resolveType(ctx);
    }

    // The result type of an assignment expression is the target's type
    this->result_type = this->target->result_type;
}

void AssignExpr::compile(CompileContext &ctx) {
    // Compile the value
    this->value->compile(ctx);

    // The variable is only compiled on read, so we don't do it now
    // We get the local index from the target variable node
    VariableNode *varNode = dynamic_cast<VariableNode *>(this->target.get());
    if (varNode == nullptr || varNode->local_index == -1) {
        throw ParserError(
            this->token,
            "Invalid assignment target during compilation.");
    }

    // Store the value into the local variable
    if (varNode->local_index > 255) {
        ctx.chunk->write(OpCode::SetLocalLong, this->token.line);
        ctx.chunk->writeWord(static_cast<uint16_t>(varNode->local_index));
    } else {
        ctx.chunk->write(OpCode::SetLocal, this->token.line);
        ctx.chunk->write(static_cast<uint8_t>(varNode->local_index));
    }
}

void AssignExpr::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "AssignExpr\n";
    this->target->print(indent + 1);
    this->value->print(indent + 1);
}

// IfStmt Implementation
IfStmt::IfStmt(const Token &token, ASTNodePtr condition, ASTNodePtr then_branch,
               ASTNodePtr else_branch)
    : ASTNode(ASTNodeType::IfStmt, token),
      condition(std::move(condition)),
      then_branch(std::move(then_branch)),
      else_branch(std::move(else_branch)) {}

void IfStmt::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // Resolve everything first
    this->condition->resolveType(ctx);
    this->then_branch->resolveType(ctx);
    if (this->else_branch) {
        this->else_branch->resolveType(ctx);
    }

    // For now, this has no result type
    this->result_type = ctx.typeRegistry.noneType();

    // Condition must be boolean
    const auto booleanType = ctx.typeRegistry.getPrimitive(PrimitiveKind::Boolean);
    if (this->condition->result_type == booleanType)
        return;

    // If not, find a cast
    auto castOp = ctx.typeRegistry.getCastOp(
        this->condition->result_type.value(), booleanType);
    if (!castOp.has_value()) {
        throw ParserError(
            this->token,
            "If statement condition should coerce to boolean type.");
    }

    this->condition = std::make_unique<CastExpr>(
        this->token,
        std::move(this->condition),
        booleanType);
    this->condition->resolveType(ctx);
}

void IfStmt::compile(CompileContext &ctx) {
    // Compile the condition first
    this->condition->compile(ctx);

    // Insert jump if false, take note of the jump address and insert dummy
    ctx.chunk->write(OpCode::JmpIfFalsePop, this->token.line);
    const auto if_jump =
        ctx.chunk->writeWord(0xFFFF); // Placeholder

    // Compile then branch
    this->then_branch->compile(ctx);
    
    // If there is an else branch, insert jump to after else
    unsigned else_jump = 0;
    if (this->else_branch) {
        ctx.chunk->write(OpCode::Jmp);
        else_jump =
            ctx.chunk->writeWord(0xFFFF); // Placeholder
    }

    // Patch first jump
    const unsigned after_then_addr = ctx.chunk->currentOffset();
    const int16_t offset_to_after_then =
        static_cast<int16_t>(after_then_addr - (if_jump + 2));
    ctx.chunk->patchWord(if_jump, offset_to_after_then);

    // If there is no else branch, we're done
    if (!this->else_branch)
        return;

    // Compile else branch
    this->else_branch->compile(ctx);

    // Patch else jump
    const unsigned after_else_addr = ctx.chunk->currentOffset();
    const int16_t offset_to_after_else =
        static_cast<int16_t>(after_else_addr - (else_jump + 2));
    ctx.chunk->patchWord(else_jump, offset_to_after_else);
}

void IfStmt::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "IfStmt\n";
    this->condition->print(indent + 1);
    this->then_branch->print(indent + 1);
    if (this->else_branch) {
        this->else_branch->print(indent + 1);
    }
}

// WhileStmt Implementation
WhileStmt::WhileStmt(const Token &token, ASTNodePtr condition, ASTNodePtr body)
    : ASTNode(ASTNodeType::WhileStmt, token),
      condition(std::move(condition)),
      body(std::move(body)) {}

void WhileStmt::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // Resolve condition and body
    this->condition->resolveType(ctx);
    this->body->resolveType(ctx);

    // While statements have no result type
    this->result_type = ctx.typeRegistry.noneType();

    // Condition must be boolean
    const auto booleanType = ctx.typeRegistry.getPrimitive(PrimitiveKind::Boolean);
    if (this->condition->result_type == booleanType)
        return;

    // If not, find a cast
    auto castOp = ctx.typeRegistry.getCastOp(
        this->condition->result_type.value(), booleanType);
    if (!castOp.has_value()) {
        throw ParserError(
            this->token,
            "While statement condition should coerce to boolean type.");
    }

    this->condition = std::make_unique<CastExpr>(
        this->token,
        std::move(this->condition),
        booleanType);
    this->condition->resolveType(ctx);
}

void WhileStmt::compile(CompileContext &ctx) {
    // Mark the beggining of the condition
    const auto before_condition = ctx.chunk->currentOffset();

    // Compile the condition
    this->condition->compile(ctx);

    // Insert jump to end of loop if condition is false
    ctx.chunk->write(OpCode::JmpIfFalsePop);
    const auto jump_to_patch = ctx.chunk->writeWord(0xFFFF);

    // Compile body
    this->body->compile(ctx);

    // Insert jump to condition
    ctx.chunk->write(OpCode::Jmp);
    const int16_t before_offset =
        static_cast<int16_t>(before_condition) -
        static_cast<int16_t>(ctx.chunk->currentOffset() + 2);
    ctx.chunk->writeWord(before_offset);

    // Patch first jump
    const unsigned final_addr = ctx.chunk->currentOffset();
    const int16_t offset_to_end =
        static_cast<int16_t>(final_addr - (jump_to_patch + 2));
    ctx.chunk->patchWord(jump_to_patch, offset_to_end);
}

void WhileStmt::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "WhileStmt\n";
    this->condition->print(indent + 1);
    this->body->print(indent + 1);
}

