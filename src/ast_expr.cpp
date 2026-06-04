#include "ast_expr.hpp"
#include "ast_macros.hpp"
#include "ast_pure_expr.hpp"
#include "error.hpp"
#include "object.hpp"
#include <iostream>

// CallExpr Implementation
CallExpr::CallExpr(const Token &token, ASTNodePtr callee,
                   std::vector<ASTNodePtr> arguments)
    : ASTNode(ASTNodeType::CallExpr, token), callee(std::move(callee)),
      arguments(std::move(arguments)) {}

void CallExpr::resolveType(CompileContext &ctx) {
    TypeGuard;

    // Resolve callee type first
    this->callee->resolveType(ctx);

    // Ensure callee is a function
    const auto type_data = ctx.typeRegistry.getTypeData(this->type(callee));
    if (!std::holds_alternative<FunctionType>(type_data)) {
        throw ParserError(this->token, "Callee isn't a function");
    }
    const FunctionType &function_type = std::get<FunctionType>(type_data);

    // The result_type of this call is the return type of the function
    type(this) = function_type.return_type;

    // Check the number of arguments is correct
    if (this->arguments.size() != function_type.param_types.size())
        throw ParserError(this->token,
                          std::format("Wrong number of arguments in function "
                                      "call. Expected {}, got {}",
                                      function_type.param_types.size(),
                                      this->arguments.size()));

    // Check the arguments one by one
    for (unsigned i = 0; i < this->arguments.size(); i++) {
        this->arguments[i]->resolveType(ctx);

        const auto arg_token = this->arguments[i]->token;
        auto cast_result = CastExpr::tryCast(std::move(this->arguments[i]),
                                             function_type.param_types[i], ctx);
        if (!cast_result.has_value())
            throw ParserError(
                arg_token,
                std::format("Incompatible type for argument {} in call",
                            i + 1));
        this->arguments[i] = std::move(cast_result.value());
    }

    // Everything ok
}

void CallExpr::compile(CompileContext &ctx, int reg) {
    CompileGuard;

    // Check if the callee is self
    bool self_call = false;
    if (this->callee->type == ASTNodeType::Variable) {
        auto var_node = static_cast<VariableNode *>(this->callee.get());
        if (var_node->name == "self") {
            self_call = true;
        }
    }

    if (!self_call) {
        // Ensure callee is in a register
        this->callee->compile(ctx);
    } else {
        reg(this->callee) = 256; // aka. first object constant;
    }

    if (this->arguments.empty()) {
        // If a register was provided, use that
        reg(this) = reg != -1 ? reg : ctx.allocateRegister();

        // Call
        // The return value is placed on the first argument register, so
        // pass that
        ctx.function->chunk.writeABC(OpCode::Call, 0, reg(callee), reg(this),
                                     this->token.line);

        // Free callee register if necessary
        if (should_free(this->callee) && !self_call) {
            // Release the callee register
            ctx.function->chunk.writeABx(OpCode::Release, this->reg(callee), 0,
                                         this->token.line);
            ctx.freeRegister(this->reg(callee));
        }

        // The result is already in the correct register, do nothing
        return;
    }

    // Get continous registers for the arguments
    std::vector<int> arg_registers;
    arg_registers.reserve(this->arguments.size());
    for (size_t i = 0; i < this->arguments.size(); i++)
        arg_registers.push_back(ctx.allocateFromTop());

    // If a register was provided, use that, if not, use the first argument
    // register
    reg(this) = reg != -1 ? reg : arg_registers[0];

    // Compile the arguments
    int i = 0;
    for (auto &arg : this->arguments)
        arg->compile(ctx, arg_registers[i++]);

    // Emit call
    ctx.function->chunk.writeABC(OpCode::Call, this->arguments.size(),
                                 reg(callee), reg(this->arguments[0]),
                                 this->token.line);

    if (reg(this) == arg_registers[0]) {
        // The result is already in the correct register, do nothing
        // Free the argument registers in reverse order (except ours)
        for (int j = this->arguments.size() - 1; j > 0; j--)
            ctx.freeRegister(arg_registers[j]);

    } else {
        // Move the result to the desired register
        ctx.function->chunk.writeABx(OpCode::Copy, reg(this), arg_registers[0],
                                     this->token.line);

        // Free the argument registers in reverse order
        for (int j = this->arguments.size() - 1; j >= 0; j--)
            ctx.freeRegister(arg_registers[j]);
    }

    // Free callee register if necessary
    if (should_free(this->callee) && !self_call) {
        // Release the callee register
        ctx.function->chunk.writeABx(OpCode::Release, this->reg(callee), 0,
                                     this->token.line);
        ctx.freeRegister(this->reg(callee));
    }

    // Fixup registers
    ctx.fixupRegisters();
}

void CallExpr::print(int indent) {
    for (int i = 0; i < indent; i++)
        std::cout << "  ";
    std::cout << "CallExpr()\n";
    this->callee->print(indent + 1);
    for (auto &arg : this->arguments)
        arg->print(indent + 1);
}

// AssignExpr Implementation
AssignExpr::AssignExpr(const Token &token, ASTNodePtr target, ASTNodePtr value)
    : ASTNode(ASTNodeType::AssignExpr, token), target(std::move(target)),
      value(std::move(value)) {}

void AssignExpr::resolveType(CompileContext &ctx) {
    TypeGuard;

    // Resolve target and value types first
    this->target->resolveType(ctx);
    this->value->resolveType(ctx);

    // Self is not a valid assignment target
    VariableNode *varNode = dynamic_cast<VariableNode *>(this->target.get());
    if (varNode->name == "self") {
        throw ParserError(this->token, "Cannot assign to 'self' variable.");
    }

    // Ensure the value can be assigned to the target
    auto cast_result =
        CastExpr::tryCast(std::move(this->value), type(this->target), ctx);
    if (!cast_result.has_value()) {
        throw ParserError(this->token,
                          "Incompatible types in assignment expression.");
    }
    this->value = std::move(cast_result.value());

    // The result type of an assignment expression is the target's type
    type(this) = type(target);
}

void AssignExpr::compile(CompileContext &ctx, int reg) {
    CompileGuard;
    // Get the variable node from the target
    VariableNode *varNode = dynamic_cast<VariableNode *>(this->target.get());

    // Ensure it's a valid assignment target
    if (!std::holds_alternative<EntryID>(varNode->resolution)) {
        throw ParserError(this->token,
                          "Invalid assignment target during compilation.");
    }

    // Compile upvalue or local variable
    const auto local_entry = std::get<EntryID>(varNode->resolution);
    const auto &entry = ctx.nameTable.getEntry(local_entry);
    if (entry.is_upvalue) {
        this->compileUpvalue(ctx, reg);
    } else {
        this->compileLocal(ctx, local_entry, reg);
    }
}

void AssignExpr::compileLocal(CompileContext &ctx, EntryID local_entry,
                              int reg) {
    const int local_register =
        ctx.nameTable.getEntry(local_entry).register_index;

    // If a register was provided, use that
    // Otherwise, use the local variable's register
    if (reg != -1) {
        reg(this) = reg;
        is_var(this) = false;
    } else {
        reg(this) = local_register;
        is_var(this) = true;
    }

    // If we are writting into an object, release the previous one first
    bool is_object = ctx.typeRegistry.isObject(type(this->target));

    if (is_object)
        ctx.function->chunk.writeABx(OpCode::Release, local_register, 0,
                                     this->token.line);

    // Compile the value into the local variable's register
    this->value->compile(ctx, local_register);

    // If we are using a different register, copy the value
    if (reg(this) != local_register) {
        // Store the local variable
        ctx.function->chunk.writeABx(OpCode::Copy, reg(this), local_register,
                                     this->token.line);

        // Because of the copy, we have to retain
        if (is_object)
            ctx.function->chunk.writeABx(OpCode::Retain, local_register, 0);
    }
}

void AssignExpr::compileUpvalue(CompileContext &ctx, int reg) {
    const auto entry_id = std::get<EntryID>(
        static_cast<VariableNode *>(this->target.get())->resolution);
    const int upvalue_index = ctx.getUpvalueIndex(entry_id);

    // If a register was provided, use that, if not, allocate a new one
    reg(this) = reg != -1 ? reg : ctx.allocateRegister();
    is_var(this) = false;

    // Compile the value into the upvalue register
    this->value->compile(ctx, reg(this));

    // Store the upvalue
    ctx.function->chunk.writeABx(OpCode::SetUpvalue, upvalue_index, reg(this),
                                 this->token.line);
}

void AssignExpr::print(int indent) {
    for (int i = 0; i < indent; i++)
        std::cout << "  ";
    std::cout << "AssignExpr\n";
    this->target->print(indent + 1);
    this->value->print(indent + 1);
}
