#include "ast_stmt.hpp"
#include "ast_macros.hpp"
#include "ast_pure_expr.hpp"
#include "error.hpp"
#include "native.hpp"
#include "object.hpp"
#include <cassert>
#include <iostream>

// ExprStmt Implementation
ExprStmt::ExprStmt(const Token &token, ASTNodePtr expression)
    : ASTNode(ASTNodeType::ExprStmt, token),
      expression(std::move(expression)) {}

void ExprStmt::resolveType(CompileContext &ctx) {
    TypeGuard;

    // Resolve the expression type if any
    if (this->expression)
        this->expression->resolveType(ctx);

    // Expression statements have no result type
    type(this) = ctx.typeRegistry.noneType();
}

void ExprStmt::compile(CompileContext &ctx, int reg) {
    CompileGuard;

    assert(reg == -1);

    // There is no result register for expression statements
    reg(this->expression) = -1;

    // If there is no expression, nothing to compile
    if (!this->expression) {
        return;
    }

    // Compile the expression
    this->expression->compile(ctx);

    // Free it if needed
    if (should_free(this->expression)) {
        ctx.freeRegister(reg(this->expression));

        // If it was an object type, release it
        if (ctx.typeRegistry.isObject(type(this->expression))) {
            ctx.function->chunk.writeABx(OpCode::Release,
                                         reg(this->expression), 0,
                                         this->token.line);
        }
    }
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
    TypeGuard;

    // Enter a new scope
    ctx.nameTable.enterScope();

    // Resolve types for all statements
    bool plain_return = false;
    for (auto &stmt : this->statements) {
        if (!plain_return) {
            stmt->resolveType(ctx);
            if (stmt->type == ASTNodeType::ReturnStmt) {
                plain_return = true;
            }
        } else {
            // Dead code after return
            throw ParserError(
                stmt->token,
                "Unreachable code after return statement.");
        }
    }

    // Exit the scope
    ctx.nameTable.exitScope();

    // Block statements have no result type
    type(this) = ctx.typeRegistry.noneType();
}

void BlockStmt::compile(CompileContext &ctx, int reg) {
    CompileGuard;
    assert(reg == -1);

    // Enter a new scope
    ctx.nameTable.enterScope();

    // Compile all statements in the block
    for (auto &stmt : this->statements) {
        stmt->compile(ctx);
    }

    // Get all local variables declared in this block
    auto names_in_scope = ctx.nameTable.getNamesInScope(
            ctx.nameTable.getCurrentDepth());

    // Pop local variables declared in this block
    for (auto entryID : names_in_scope) {
        const auto &entry = ctx.nameTable.getEntry(entryID);
        if (entry.register_index != -1) {
            // Free the register
            ctx.freeRegister(entry.register_index);

            // If its an object type, release it
            if (ctx.typeRegistry.isObject(entry.type)) {
                ctx.function->chunk.writeABx(
                    OpCode::Release, entry.register_index,
                    0, this->token.line);
            }
        }
    };

    // Exit the scope
    ctx.nameTable.exitScope();
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
                         ASTNodePtr initializer, bool is_in_function_definition)
    : ASTNode(ASTNodeType::VarDeclStmt, name_token),
      type_expr(std::move(type_expr)), initializer(std::move(initializer)),
      is_in_function_definition(is_in_function_definition) {}

void VarDeclStmt::resolveType(CompileContext &ctx) {
    TypeGuard;

    // Check if the variable name is 'self'
    if (this->token.lexeme == "self") {
        throw ParserError(
            this->token,
            "Cannot declare variable with reserved name 'self'.");
    }

    // Check if its a native function
    auto nativeFn = ctx.nativeRegistry.getNativeFunction(
        this->token.lexeme);
    if (nativeFn != nullptr) {
        throw ParserError(
            this->token,
            "Variable name conflicts with a native function name.");
    }

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

        type(this) = type(initializer);
    } else {
        // Otherwise, use the declared type
        type(this) = type(type_expr);

        // If there is an initializer, ensure it matches the declared type
        if (this->initializer) {
            // Cast them if necessary
            auto cast_result = CastExpr::tryCast(
                std::move(this->initializer),
                type(this),
                ctx);
            if (!cast_result.has_value()) {
                throw ParserError(
                    this->token,
                    "Incompatible types in variable initializer.");
            }
            this->initializer = std::move(cast_result.value());
        } else if (ctx.typeRegistry.isObject(type(this))) {
            // Functions require an initializer outside of function definitions
            if (!this->is_in_function_definition && ctx.typeRegistry.isFunction(type(this))) {
                throw ParserError(
                    this->token,
                    "Function declarations require an initializer.");
            }
        }
    }

    // Add a new local
    auto entry_id = ctx.nameTable.addName(
        this->token.lexeme, this->token.line,
        type(this));
    if (!entry_id.has_value()) {
        throw ParserError(
            this->token,
            "Variable with the same name already declared in this scope.");
    }

    this->entry_id = entry_id.value();
}

void VarDeclStmt::compile(CompileContext &ctx, int reg) {
    CompileGuard;
    assert(reg == -1);

    // This is a pure statement, no result register
    reg(this) = -1;

    // Get the local entry
    auto &entry = ctx.nameTable.getEntry(this->entry_id);

    // Allocate a register for it and put it in current scope
    assert(entry.register_index == -1);
    entry.register_index = ctx.allocateRegister();
    ctx.nameTable.putInScope(this->entry_id);

    // Compile the initializer if present
    if (this->initializer)
        this->initializer->compile(ctx, entry.register_index);
    else {
        // If no initializer and it's an string, initialize to empty string
        const auto string_type =
            ctx.typeRegistry.getPrimitive(PrimitiveKind::String);
        if (type(this) == string_type) {
            // Create an empty string literal
            StringObj *empty_string_obj = new StringObj("");

            // Put it into the chunk and store in the variable's register
            const auto const_index =
                ctx.function->chunk.addObjectConstant(empty_string_obj);
            ctx.function->chunk.writeABx(OpCode::Object, entry.register_index,
                                         const_index, this->token.line);
        }
    }
}

void VarDeclStmt::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "VarDeclStmt(" << this->token.lexeme << " : "
              << type(this) << ")\n";
    if (this->initializer) {
        this->initializer->print(indent + 1);
    }
}

// IfStmt Implementation
IfStmt::IfStmt(const Token &token, ASTNodePtr condition, ASTNodePtr then_branch,
               ASTNodePtr else_branch)
    : ASTNode(ASTNodeType::IfStmt, token),
      condition(std::move(condition)),
      then_branch(std::move(then_branch)),
      else_branch(std::move(else_branch)) {}

void IfStmt::resolveType(CompileContext &ctx) {
    TypeGuard;

    // Resolve everything first
    this->condition->resolveType(ctx);
    this->then_branch->resolveType(ctx);
    if (this->else_branch) {
        this->else_branch->resolveType(ctx);
    }

    // For now, this has no result type
    type(this) = ctx.typeRegistry.noneType();

    // Condition must be boolean
    const auto booleanType = ctx.typeRegistry.getPrimitive(PrimitiveKind::Boolean);
    auto cast_result = CastExpr::tryCast(
        std::move(this->condition), booleanType, ctx);
    if (!cast_result.has_value()) {
        throw ParserError(
            this->token,
            "If statement condition should coerce to boolean type.");
    }
    this->condition = std::move(cast_result.value());
}

void IfStmt::compile(CompileContext &ctx, int reg) {
    CompileGuard;
    assert(reg == -1);
    reg(this) = -1;

    // Compile the condition first
    this->condition->compile(ctx);

    // Insert jump if false, take note of the jump address and insert dummy
    const auto if_jump = ctx.function->chunk.writeAsBx(
        OpCode::JmpIfFalse, reg(this->condition), 0xFFFF, this->token.line);

    // Free condition register if needed
    if (should_free(this->condition))
        ctx.freeRegister(reg(this->condition));

    // Compile then branch
    this->then_branch->compile(ctx);
    
    // If there is an else branch, insert jump to after else
    unsigned else_jump = 0;
    if (this->else_branch)
        else_jump = ctx.function->chunk.writeABx(OpCode::Jmp, 0, 0xFFFF);

    // Patch first jump
    const unsigned after_then_addr = ctx.function->chunk.currentOffset();
    const int16_t offset_to_after_then =
        static_cast<int16_t>(after_then_addr - if_jump - 1);
    ctx.function->chunk.patch_AsBx(if_jump, offset_to_after_then);

    // If there is no else branch, we're done
    if (!this->else_branch)
        return;

    // Compile else branch
    this->else_branch->compile(ctx);

    // Patch else jump
    const unsigned after_else_addr = ctx.function->chunk.currentOffset();
    const int16_t offset_to_after_else =
        static_cast<int16_t>(after_else_addr - else_jump - 1);
    ctx.function->chunk.patch_AsBx(else_jump, offset_to_after_else);
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
    TypeGuard;

    // Resolve condition and body
    this->condition->resolveType(ctx);
    this->body->resolveType(ctx);

    // While statements have no result type
    type(this) = ctx.typeRegistry.noneType();

    // Condition must be boolean
    const auto booleanType = ctx.typeRegistry.getPrimitive(PrimitiveKind::Boolean);
    auto cast_result = CastExpr::tryCast(
        std::move(this->condition), booleanType, ctx);
    if (!cast_result.has_value()) {
        throw ParserError(
            this->token,
            "While statement condition should coerce to boolean type.");
    }
    this->condition = std::move(cast_result.value());
}

void WhileStmt::compile(CompileContext &ctx, int reg) {
    CompileGuard;
    assert(reg == -1);
    reg(this) = -1;

    // Mark the beggining of the condition
    const auto before_condition = ctx.function->chunk.currentOffset();

    // Compile the condition
    this->condition->compile(ctx);

    // Insert jump to end of loop if condition is false
    const auto jump_to_patch = ctx.function->chunk.writeAsBx(
        OpCode::JmpIfFalse, reg(this->condition), 0xFFFF, this->token.line);

    // Free condition register if needed
    if (should_free(this->condition))
        ctx.freeRegister(reg(this->condition));

    // Compile body
    this->body->compile(ctx);

    // Insert jump to condition
    const int16_t before_offset =
        static_cast<int16_t>(before_condition) -
        static_cast<int16_t>(ctx.function->chunk.currentOffset()) - 1;
    ctx.function->chunk.writeAsBx(OpCode::Jmp, 0, before_offset);

    // Patch first jump
    const unsigned final_addr = ctx.function->chunk.currentOffset();
    const int16_t offset_to_end =
        static_cast<int16_t>(final_addr - jump_to_patch - 1);
    ctx.function->chunk.patch_AsBx(jump_to_patch, offset_to_end);
}

void WhileStmt::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "WhileStmt\n";
    this->condition->print(indent + 1);
    this->body->print(indent + 1);
}

// ReturnStmt
ReturnStmt::ReturnStmt(const Token &token, ASTNodePtr return_expr)
    : ASTNode(ASTNodeType::ReturnStmt, token),
      return_expr(std::move(return_expr)) {};

void ReturnStmt::resolveType(CompileContext &ctx) {
    TypeGuard;

    // Ensure this is happening in a function
    if (!ctx.next)
        throw ParserError(this->token,
            "Return statement outside function");

    // The return type of the statement is none
    type(this) = ctx.typeRegistry.noneType();

    // If there is expr, compile it
    if (this->return_expr) {
        this->return_expr->resolveType(ctx);

        // Check for types
        const auto function_type_data =
            ctx.typeRegistry.getTypeData(ctx.function->type_id);
        assert(std::holds_alternative<FunctionType>(function_type_data));
        TypeID return_type = std::get<FunctionType>(function_type_data).return_type;

        auto cast_result = CastExpr::tryCast(
            std::move(this->return_expr), return_type, ctx);
        if (!cast_result.has_value()) {
            throw ParserError(
                this->token,
                "Return type doesn't match with function declaration");
        }
        this->return_expr = std::move(cast_result.value());
    }
}

void ReturnStmt::compile(CompileContext &ctx, int reg) {
    CompileGuard;

    // This is a pure statement, no result register
    assert(reg == -1);

    // If there is expression, compile that
    if (this->return_expr) {
        this->return_expr->compile(ctx, reg);
        reg = reg(this->return_expr);

        // Try to skip constant
        SKIP_CONSTANT_GET_REG(reg);

    } else
        reg = 0;

    // Get all local variables defined now
    auto all_names = ctx.nameTable.getNamesInScope(0);

    // Release all object locals
    for (auto entryID : all_names) {
        const auto &entry = ctx.nameTable.getEntry(entryID);
        if (entry.register_index != -1 &&
            ctx.typeRegistry.isObject(entry.type)) {
            ctx.function->chunk.writeABx(
                OpCode::Release, entry.register_index,
                0, this->token.line);
        }
    };
    
    // Finally, return
    ctx.function->chunk.writeABx(
        OpCode::Return, 0, reg, this->token.line);
}

void ReturnStmt::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "ReturnStmt\n";
    if (this->return_expr)
        this->return_expr->print(indent + 1);
}

