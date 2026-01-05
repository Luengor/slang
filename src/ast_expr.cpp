#include "ast_expr.hpp"
#include "ast_stmt.hpp"
#include "compile_context.hpp"
#include "error.hpp"
#include "native.hpp"
#include "object.hpp"
#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <variant>

// overload boilerplate
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

#define ResolveGuard \
    if (this->result_type.has_value()) { \
        return; \
    }

// LiteralNode Implementation
LiteralNode::LiteralNode(const Token &token)
    : ASTNode(ASTNodeType::Literal, token) {
    // Parse the value to check what it is
    switch (token.type) {
        case Token::Type::Number: {
            if (token.lexeme.find('.') == std::string::npos) {
                // It's an integer (fixed)
                long long number = std::stoll(token.lexeme);
                Value val {.fixed = number};
                this->value = std::make_pair(ValueType::Fixed, val);
            } else {
                // It's a floating-point number
                double number = std::stod(token.lexeme);
                Value val {.floating = number};
                this->value = std::make_pair(ValueType::Floating, val);
            }
            break;
        }

        case Token::Type::True: {
            Value val {.boolean = true};
            this->value = std::make_pair(ValueType::Boolean, val);
            break;
        }

        case Token::Type::False: {
            Value val {.boolean = false};
            this->value = std::make_pair(ValueType::Boolean, val);
            break;
        }

        case Token::Type::String: {
            // It's a string literal
            // Remove the surrounding quotes
            std::string strValue = token.lexeme.substr(1, token.lexeme.length() - 2);

            // Create a StringObj
            StringObj *strObj = new StringObj(strValue);

            Value val {.object = strObj};
            this->value = std::make_pair(ValueType::Object, val);
            break;
        }

        default:
            throw ParserError(token, "Unsupported literal type.");
    }
}

void LiteralNode::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // Get result type of the literal
    this->result_type = ctx.typeRegistry.getFromValue(this->value);
    if (result_type == ctx.typeRegistry.noneType()) {
        throw ParserError(this->token,
                          "Unknown literal type during type resolution.");
    }
}

void LiteralNode::compile(CompileContext &ctx, int reg) {
    // Allocate a register for the result
    this->result_register = reg == -1 ? ctx.allocateRegister() : reg;

    if (ctx.typeRegistry.isObject(this->result_type.value())) {
        // Object constant
        const auto constant =
            ctx.function->chunk.addObjectConstant(this->value.second.object);
        ctx.function->chunk.write_Ab(OpCode::Object, constant,
                                     this->result_register, this->token.line);
        return;
    }

    // Add a constant to the chunk
    const auto constant = ctx.function->chunk.addConstant(this->value.second);

    // Write the constant load instruction
    ctx.function->chunk.write_Ab(OpCode::Constant, constant,
                                 this->result_register, this->token.line);
}

void LiteralNode::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    switch (this->value.first) {
        case ValueType::Floating:
            std::println("Literal({:g})", this->value.second.floating);
            break;

        case ValueType::Fixed:
            std::println("Literal({:d})", this->value.second.fixed);
            break;

        case ValueType::Boolean:
            std::println("Literal({})", this->value.second.boolean);
            break;

        case ValueType::Object:
            print_object();
            break;

        default:
            std::println("Literal(Unknown Type)");
            break;
    }
}

void LiteralNode::print_object() {
    switch (this->value.second.object->type) {
        case Object::Type::String: {
            StringObj *strObj = static_cast<StringObj *>(this->value.second.object);
            std::println("Literal(\"{}\")", strObj->value);
            break;
        }

        case Object::Type::Function: {
            std::println("Literal(<Function Object>)");
            break;
        }

        case Object::Type::NativeFunction: {
            NativeFunctionObj *nativeFn =
                static_cast<NativeFunctionObj *>(this->value.second.object);
            std::println("Literal(<Native Function: {}>)", nativeFn->name);
            break;
        }
    }
}

// FunctionNode Implementation
FunctionNode::FunctionNode(const Token &token,
                           std::vector<ASTNodePtr> arguments,
                           ASTNodePtr return_type, ASTNodePtr body)
    : ASTNode(ASTNodeType::Function, token), arguments(std::move(arguments)),
      return_type(std::move(return_type)), body(std::move(body)) {}

void FunctionNode::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // Create a new compile context for the function
    this->fn_ctx = std::make_unique<CompileContext>(ctx);

    // Start compiling the function
    this->fn_ctx->function = new FunctionObj();

    // Name the function after its line number for now
    this->fn_ctx->function->name =
        "<fn@" + std::to_string(this->token.line) + ">";

    // Add a self local for recursion
    this->self_entry_id = this->fn_ctx->nameTable.addName(
        "self", this->token.line, ctx.typeRegistry.noneType()).value();

    // Resolve argument types
    std::vector<TypeID> param_types;
    for (const auto &arg : this->arguments) {
        arg->resolveType(*fn_ctx);
        param_types.push_back(arg->result_type.value());
    }

    // Resolve result type
    this->return_type->resolveType(*fn_ctx);
    TypeID return_type_id = this->return_type->result_type.value();

    // Get the function type ID
    this->result_type = ctx.typeRegistry.getFunction(
        param_types, return_type_id);

    this->fn_ctx->function->type_id = this->result_type.value();

    // Update self type 
    this->fn_ctx->nameTable.getEntry(this->self_entry_id).type =
        this->result_type.value();

    // Check if the last statement in the body is a return statement
    assert(this->body->type == ASTNodeType::BlockStmt);
    auto body_block = static_cast<BlockStmt *>(this->body.get());
    if (body_block->statements.empty() ||
        body_block->statements.back()->type != ASTNodeType::ReturnStmt) {
        // Put an implicit return at the end
        body_block->statements.push_back(std::make_unique<ReturnStmt>(
            body_block->statements.empty() ? this->token
                                        : body_block->statements.back()->token,
            nullptr
        ));
    }

    // Resolve type of the body
    this->body->resolveType(*fn_ctx);
}

void FunctionNode::compile(CompileContext &ctx, int reg) {
    // Use the function's own compile context
    CompileContext &fn_ctx = *this->fn_ctx;

    // Compile the args to push the locals
    for (const auto &arg : this->arguments) {
        arg->compile(fn_ctx); // args don't need registers
    }

    this->body->compile(fn_ctx); // the body doesn't need a register

#ifdef DEBUG_PRINT
    // Debug: print the compiled function bytecode and name table
    fn_ctx.nameTable.printTable();

    fn_ctx.function->chunk.disassemble(
        "Function@" + std::to_string(this->token.line));
#endif

    // Add the function object as a constant to the main chunk
    const auto constant =
        ctx.function->chunk.addObjectConstant(fn_ctx.function);

    // Write the object load instruction
    this->result_register = reg == -1 ? ctx.allocateRegister() : reg;
    ctx.function->chunk.write_Ab(
        OpCode::Object, constant, this->result_register, this->token.line);
}

void FunctionNode::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "Function(\n";
    for (const auto &arg : this->arguments) {
        arg->print(indent + 1);
    }
    this->body->print(indent + 1);
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << ")\n";
}

// Variable Node Implementation
VariableNode::VariableNode(const Token &token, const std::string &name)
    : ASTNode(ASTNodeType::Variable, token), name(name) {}

// Resolve type is only called on variable use
void VariableNode::resolveType(CompileContext &ctx) {
    ResolveGuard;

    /// Resolve the name
    // Check if it is a native function first
    auto nativeFn =
        ctx.nativeRegistry.getNativeFunction(this->name);
    if (nativeFn != nullptr) {
        this->resolution = nativeFn;
        this->result_type = nativeFn->type_id;
        return;
    }

    // If not, resolve as a variable
    auto entry = ctx.nameTable.findEntryInScope(this->name);
    if (!entry.has_value()) {
        throw ParserError(this->token,
                          "Undefined variable: " + this->name);
    }

    this->resolution = entry.value();
    this->result_type = ctx.nameTable.getEntry(entry.value()).type;
}

void VariableNode::compile(CompileContext &ctx, int reg) {
    // Get a register for the result
    this->result_register = reg == -1 ? ctx.allocateRegister() : reg;

    // Compile based on the resolution type
    std::visit(overloaded{
        [&](EntryID local_index) {
            // Get the local
            const auto &entry = ctx.nameTable.getEntry(local_index);
            assert(entry.register_index != -1 &&
                   "Local variable must have a valid register index");

            // Copy the local variable into the result register
            ctx.function->chunk.write_AB(
                OpCode::Copy, static_cast<uint8_t>(entry.register_index),
                this->result_register, this->token.line);

            // If its an object type, retain it
            if (ctx.typeRegistry.isObject(this->result_type.value())) {
                ctx.function->chunk.write_Ab(
                    OpCode::Retain, this->result_register,
                    0, this->token.line);
            }
        },
        [&](NativeFunctionObj *native_fn) {
            // Retain the native function to pass it to the chunk
            native_fn->retain();

            // Load the native function as a constant
            const auto constant =
                ctx.function->chunk.addObjectConstant(native_fn);
            ctx.function->chunk.write_Ab(
                OpCode::Object, constant,
                this->result_register, this->token.line);
        }
    }, this->resolution);
}

void VariableNode::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "Variable(" << this->name << ")\n";
}


// UnaryExpr Implementation
UnaryExpr::UnaryExpr(const Token &token, ASTNodePtr operand)
    : ASTNode(ASTNodeType::UnaryExpr, token), operand(std::move(operand)) {}

void UnaryExpr::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // Resolve the operand type first
    this->operand->resolveType(ctx);
    auto operand_type = this->operand->result_type;

    // Determine the result type based on the type and operator
    const auto fixedType = ctx.typeRegistry.getPrimitive(PrimitiveKind::Fixed);
    const auto floatingType = ctx.typeRegistry.getPrimitive(PrimitiveKind::Floating);
    const auto booleanType = ctx.typeRegistry.getPrimitive(PrimitiveKind::Boolean);

    switch (this->token.type) {
        case Token::Type::Minus:
            if (operand_type == fixedType || operand_type == floatingType) {
                this->result_type = operand_type;
            } else {
                throw ParserError(
                    this->token,
                    "Unary minus operator requires a numeric operand.");
            }
            break;

        case Token::Type::Not:
            if (operand_type == booleanType) {
                this->result_type = booleanType;
            } else {
                throw ParserError(
                    this->token,
                    "Unary not operator requires a boolean operand.");
            }
            break;

        default:
            throw ParserError(
                this->token,
                "Unsupported unary operator during type resolution.");
            break;
    }
}

void UnaryExpr::compile(CompileContext &ctx, int reg) {
    // Get a register for the result
    this->result_register = reg == -1 ? ctx.allocateRegister() : reg;

    // Compile the operand first
    this->operand->compile(ctx, this->result_register);

    // Compile the appropriate unary operation
    switch (this->token.type) {
        case Token::Type::Minus:
            if (this->result_type ==
                ctx.typeRegistry.getPrimitive(PrimitiveKind::Fixed)) {
                ctx.function->chunk.write_AB(
                    OpCode::NegateI, this->result_register,
                    this->result_register, this->token.line);
            } else {
                ctx.function->chunk.write_AB(
                    OpCode::NegateF, this->result_register,
                    this->result_register, this->token.line);
            }
            break;

        case Token::Type::Not:
            ctx.function->chunk.write_AB(
                OpCode::Not, this->result_register,
                this->result_register, this->token.line);
            break;

        default:
            throw ParserError(
                this->token,
                "Unsupported unary operator during compilation.");
            break;
    }
}

void UnaryExpr::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "UnaryExpr(" << this->token.lexeme << ")\n";
    this->operand->print(indent + 1);
}

// CastExpr Implementation
CastExpr::CastExpr(const Token &token, ASTNodePtr operand, TypeID target_type)
    : ASTNode(ASTNodeType::CastExpr, token),
      operand(std::move(operand)), target_type(target_type) {
}

std::optional<ASTNodePtr> CastExpr::tryCast(ASTNodePtr operand,
                                            TypeID target_type,
                                            CompileContext &ctx) {
    assert(operand->result_type.has_value() && "Operand type must be resolved");
    if (operand->result_type.value() == target_type) {
        // No cast needed
        return std::make_optional<ASTNodePtr>(std::move(operand));
    }

    auto castOp = ctx.typeRegistry.getCastOp(
        operand->result_type.value(), target_type);

    if (!castOp.has_value()) return std::nullopt;
    auto castExpr = std::make_unique<CastExpr>(
        operand->token,
        std::move(operand),
        target_type);
    castExpr->resolveType(ctx);
    return std::make_optional<ASTNodePtr>(std::move(castExpr));
}

std::optional<std::pair<ASTNodePtr, ASTNodePtr>>
CastExpr::tryCommonCast(ASTNodePtr left, ASTNodePtr right,
                        CompileContext &ctx) {
    assert(left->result_type.has_value() && "Left type must be resolved");
    assert(right->result_type.has_value() && "Right type must be resolved");

    // If types are already the same, no casts needed
    if (left->result_type.value() == right->result_type.value()) {
        return std::make_optional<std::pair<ASTNodePtr, ASTNodePtr>>(
            std::make_pair(std::move(left), std::move(right)));
    }

    // Try casting left to right's type
    auto leftCastOp = ctx.typeRegistry.getCastOp(
        left->result_type.value(), right->result_type.value());
    if (leftCastOp.has_value()) {
        auto castedLeft = std::make_unique<CastExpr>(
            left->token,
            std::move(left),
            right->result_type.value());
        castedLeft->resolveType(ctx);
        return std::make_optional<std::pair<ASTNodePtr, ASTNodePtr>>(
            std::make_pair(std::move(castedLeft), std::move(right)));
    }

    // Try casting right to left's type
    auto rightCastOp = ctx.typeRegistry.getCastOp(
        right->result_type.value(), left->result_type.value());
    if (rightCastOp.has_value()) {
        auto castedRight = std::make_unique<CastExpr>(
            right->token,
            std::move(right),
            left->result_type.value());
        castedRight->resolveType(ctx);
        return std::make_optional<std::pair<ASTNodePtr, ASTNodePtr>>(
            std::make_pair(std::move(left), std::move(castedRight)));
    }

    return std::nullopt;
}

void CastExpr::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // "Dodge" the guard for target type
    this->result_type = this->target_type;

    // The operand should already be resolved, just ensure it's done
    this->operand->resolveType(ctx);

    // Check if the cast is valid
    auto castOp = ctx.typeRegistry.getCastOp(
        this->operand->result_type.value(), this->result_type.value());

    if (!castOp.has_value()) {
        throw ParserError(
            this->token,
            "Invalid cast from source type to target type.");
    }

    this->cast_op = castOp.value();
}

void CastExpr::compile(CompileContext &ctx, int reg) {
    this->result_register = reg == -1 ? ctx.allocateRegister() : reg;

    // Compile the operand first
    this->operand->compile(ctx, this->result_register);

    // Write the cast operation
    ctx.function->chunk.write_AB(
        this->cast_op, this->result_register,
        this->result_register, this->token.line);
}

void CastExpr::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "Cast(to type " << this->result_type.value() << ")\n";
    this->operand->print(indent + 1);
}

// BinaryExpr Implementation
BinaryExpr::BinaryExpr(const Token &token, ASTNodePtr left,
                                           ASTNodePtr right)
    : ASTNode(ASTNodeType::BinaryExpr, token), left(std::move(left)),
      right(std::move(right)) { }

void BinaryExpr::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // Resolve left and right operand types first
    this->left->resolveType(ctx);
    this->right->resolveType(ctx);

    const auto ltype = ctx.typeRegistry.getTypeData(this->left->result_type.value());
    const auto rtype = ctx.typeRegistry.getTypeData(this->right->result_type.value());

    // Visit
    auto result_type = std::visit(overloaded{
            [&](PrimitiveType l, PrimitiveType r) -> std::optional<TypeID> {
                // Both are primitive types, find common type
                auto common = ctx.typeRegistry.getCommonPrimitive(l.kind, r.kind);
                if (common.has_value()) {
                    return ctx.typeRegistry.getPrimitive(common.value());
                }

                return std::nullopt;
            },

            [&](auto &, auto &) -> std::optional<TypeID> {
                // At least one is not a primitive type, for now we don't support this
                return std::nullopt;
            }
    }, ltype, rtype);

    // Throw if no common type found
    if (!result_type.has_value()) {
        throw ParserError(
            this->token,
            "Incompatible types in binary expression.");
    }

    // Cast operands to the common type if necessary
    if (this->left->result_type != result_type.value()) {
        this->left = std::make_unique<CastExpr>(
            this->token,
            std::move(this->left),
            result_type.value());
        this->left->resolveType(ctx);
    }

    if (this->right->result_type != result_type.value()) {
        this->right = std::make_unique<CastExpr>(
            this->token,
            std::move(this->right),
            result_type.value());
        this->right->resolveType(ctx);
    }

    // Get final result type
    switch (this->token.type) {
        case Token::Type::Plus:
        case Token::Type::Minus:
        case Token::Type::Star:
        case Token::Type::Slash:
            // Arithmetic operations yield the common numeric type
            this->result_type = result_type.value();
            break;

        case Token::Type::EqualEqual:
        case Token::Type::BangEqual:
        case Token::Type::Greater:
        case Token::Type::GreaterEqual:
        case Token::Type::Less:
        case Token::Type::LessEqual:
            // Comparison operations yield boolean type
            this->result_type = ctx.typeRegistry.getPrimitive(PrimitiveKind::Boolean);
            break;

        default:
            throw ParserError(
                this->token,
                "Unsupported binary operator during type resolution.");
            break;
    }
}

void BinaryExpr::compile(CompileContext &ctx, int reg) {
    // Get a register for the result
    this->result_register = reg == -1 ? ctx.allocateRegister() : reg;

    // Compile left and right operands first
    this->left->compile(ctx, this->result_register);
    this->right->compile(ctx);

    // Free the right operand's register
    ctx.freeRegister(this->right->result_register);

    // Compile the appropriate binary operation
    switch (this->token.type) {
        case Token::Type::Plus:
        case Token::Type::Minus:
        case Token::Type::Star:
        case Token::Type::Slash:
            return compileArithmetic(ctx);

        case Token::Type::EqualEqual:
        case Token::Type::BangEqual:
            return compileEquality(ctx);

        case Token::Type::Greater:
        case Token::Type::GreaterEqual:
        case Token::Type::Less:
        case Token::Type::LessEqual:
            return compileComparison(ctx);

        default:
            throw ParserError(
                this->token,
                "Unsupported binary operator during compilation.");
            break;
    }
}

#define write(op)                                                              \
    ctx.function->chunk.write_abc(op, this->result_register,                   \
                                  this->right->result_register,                \
                                  this->result_register, this->token.line);    \
    break;

void BinaryExpr::compileArithmetic(CompileContext &ctx) {
    const TypeID floatingType =
        ctx.typeRegistry.getPrimitive(PrimitiveKind::Floating);

    // Check that the type is numeric
    if (!ctx.typeRegistry.isNumeric(this->result_type.value())) {
        throw ParserError(
            this->token,
            "Arithmetic operators require numeric operand types.");
    }

    // Compile the appropriate arithmetic operation
#define IorF(op)                                                               \
    this->result_type == floatingType ? OpCode::op##F : OpCode::op##I

    switch (this->token.type) {
        case Token::Type::Plus:
            write(IorF(Add));

        case Token::Type::Minus:
            write(IorF(Subtract));

        case Token::Type::Star:
            write(IorF(Multiply));

        case Token::Type::Slash:
            write(IorF(Divide));

        default:
            throw ParserError(
                this->token, "Unsupported binary operator during compilation.");
            break;
    }

#undef IorF
}

void BinaryExpr::compileEquality(CompileContext &ctx) {
    const auto type_data =
        ctx.typeRegistry.getTypeData(this->left->result_type.value());

    if (!std::holds_alternative<PrimitiveType>(type_data)) {
        throw ParserError(
            this->token,
            "Comparison operators require primitive operand types.");
    }

    const PrimitiveType prim_type =
        std::get<PrimitiveType>(type_data);

#define EqOrNe(op)                                                             \
    this->token.type == Token::Type::EqualEqual ? OpCode::Eq##op               \
                                                : OpCode::Ne##op

    switch (prim_type.kind) {
        case PrimitiveKind::Fixed:
            write(EqOrNe(I));

        case PrimitiveKind::Floating:
            write(EqOrNe(F));

        case PrimitiveKind::Boolean:
            write(EqOrNe(B));

        default:
            throw ParserError(
                this->token,
                "Unsupported primitive type for comparison operators.");
            break;
    }
#undef EqOrNe
}

void BinaryExpr::compileComparison(CompileContext &ctx) {
    const auto type_data =
        ctx.typeRegistry.getTypeData(this->left->result_type.value());

    if (!std::holds_alternative<PrimitiveType>(type_data)) {
        throw ParserError(
            this->token,
            "Comparison operators require primitive operand types.");
    }

    const PrimitiveType prim_type =
        std::get<PrimitiveType>(type_data);

    if (prim_type.kind == PrimitiveKind::Boolean)
        throw ParserError(
            this->token,
            "Comparison operators do not support boolean operand types.");


#define IorF(op)                                                               \
    prim_type.kind == PrimitiveKind::Floating ? OpCode::op##F : OpCode::op##I

    switch (this->token.type) {
        case Token::Type::Greater:
            write(IorF(Gt));

        case Token::Type::GreaterEqual:
            write(IorF(Ge));

        case Token::Type::Less:
            write(IorF(Lt));

        case Token::Type::LessEqual:
            write(IorF(Le));

        default:
            throw ParserError(
                this->token,
                "Unsupported comparison operator during compilation.");
            break;
    }
#undef IorF
}

#undef write

void BinaryExpr::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "BinaryExpr(" << this->token.lexeme << ")\n";
    this->left->print(indent + 1);
    this->right->print(indent + 1);
}

// TernaryExpr Implementation
TernaryExpr::TernaryExpr(const Token &token, ASTNodePtr condition,
                                         ASTNodePtr then_branch,
                                         ASTNodePtr else_branch)
    : ASTNode(ASTNodeType::TernaryExpr, token), condition(std::move(condition)),
      then_branch(std::move(then_branch)), else_branch(std::move(else_branch)) {}

void TernaryExpr::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // Resolve condition type first
    this->condition->resolveType(ctx);

    // It must be boolean
    const auto booleanType = ctx.typeRegistry.getPrimitive(PrimitiveKind::Boolean);
    auto cast_result = CastExpr::tryCast(
        std::move(this->condition), booleanType, ctx);

    if (!cast_result.has_value()) {
        throw ParserError(
            this->token,
            "Ternary condition should coerce to boolean type.");
    }
    this->condition = std::move(cast_result.value());

    // Resolve then and else branch types
    this->then_branch->resolveType(ctx);
    this->else_branch->resolveType(ctx);

    // Make them common type if necessary
    auto common_cast = CastExpr::tryCommonCast(
        std::move(this->then_branch),
        std::move(this->else_branch),
        ctx);
    if (!common_cast.has_value()) {
        throw ParserError(
            this->token,
            "Incompatible types in ternary expression branches.");
    }

    this->then_branch = std::move(common_cast.value().first);
    this->else_branch = std::move(common_cast.value().second);

    // Set result type
    this->result_type = this->then_branch->result_type;
}

void TernaryExpr::compile(CompileContext &ctx, int reg) {/*
    // Compile condition first
    this->condition->compile(ctx, int reg);

    // Jump if false to else branch
    ctx.function->chunk.write(OpCode::JmpIfFalsePop, this->token.line);
    const int16_t jmp_to_else_pos = ctx.function->chunk.writeWord(0xFFFF);

    // Compile then branch
    this->then_branch->compile(ctx, int reg);

    // Jump to after else branch
    ctx.function->chunk.write(OpCode::Jmp, this->token.line);
    const int16_t jmp_after_else_pos = ctx.function->chunk.writeWord(0xFFFF);

    // Patch jump to else branch
    const int16_t else_pos =
        static_cast<int16_t>(ctx.function->chunk.currentOffset());
    const int16_t offset_to_else = else_pos - (jmp_to_else_pos + 2);
    ctx.function->chunk.patchWord(jmp_to_else_pos, offset_to_else);

    // Compile else branch
    this->else_branch->compile(ctx, int reg);

    // Patch jump to after else branch
    const int16_t after_else_pos =
        static_cast<int16_t>(ctx.function->chunk.currentOffset());
    const int16_t offset_to_after_else =
        after_else_pos - (jmp_after_else_pos + 2);
    ctx.function->chunk.patchWord(jmp_after_else_pos, offset_to_after_else);
*/}

void TernaryExpr::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "TernaryExpr(? : )\n";
    this->condition->print(indent + 1);
    this->then_branch->print(indent + 1);
    this->else_branch->print(indent + 1);
}

// LogicExpr
LogicExpr::LogicExpr(const Token &token, ASTNodePtr left, ASTNodePtr right)
    : ASTNode(ASTNodeType::LogicExpr, token), left(std::move(left)),
      right(std::move(right)) {}

void LogicExpr::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // Resolve left and right operand types first
    this->left->resolveType(ctx);
    this->right->resolveType(ctx);

    const auto booleanType = ctx.typeRegistry.getPrimitive(PrimitiveKind::Boolean);

    // Both operands must be boolean
    if (this->left->result_type != booleanType ||
        this->right->result_type != booleanType) {
        throw ParserError(
            this->token,
            "Logical expressions require boolean operand types.");
    }

    this->result_type = booleanType;
}

void LogicExpr::compile(CompileContext &ctx, int reg) {
    // Get a register for the result
    this->result_register = reg == -1 ? ctx.allocateRegister() : reg;

    // Compile based on the operator
    if (this->token.type == Token::Type::And) {
        return compileAnd(ctx);
    } else if (this->token.type == Token::Type::Or) {
        return compileOr(ctx);
    }
}

void LogicExpr::compileAnd(CompileContext &ctx) {
    // Compile first operand
    this->left->compile(ctx, this->result_register);

    // If that operand is false, we can skip the second operand
    const int16_t jmp_pos = ctx.function->chunk.write_sAb(
        OpCode::JmpIfFalse, 0xFFFF, this->result_register, this->token.line);

    // Compile second operand over it
    this->right->compile(ctx, this->result_register);

    // Patch the jump position
    const int16_t after_pos = static_cast<int16_t>(ctx.function->chunk.currentOffset());
    const int16_t offset = after_pos - jmp_pos - 1;
    ctx.function->chunk.patch_sA(jmp_pos, offset);
}

void LogicExpr::compileOr(CompileContext &ctx) {
    // Compile first operand
    this->left->compile(ctx, this->result_register);

    // If its true, we can skip evaluating the second operand
    const int16_t jmp_pos = ctx.function->chunk.write_sAb(
        OpCode::JmpIfTrue, 0xFFFF, this->result_register, this->token.line);

    // Compile second operand
    this->right->compile(ctx, this->result_register);

    // Patch the jump position
    const int16_t after_pos =
        static_cast<int16_t>(ctx.function->chunk.currentOffset());
    const int16_t offset = after_pos - jmp_pos - 1;
    ctx.function->chunk.patch_sA(jmp_pos, offset);
}

void LogicExpr::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "LogicExpr(" << this->token.lexeme << ")\n";
    this->left->print(indent + 1);
    this->right->print(indent + 1);
}

// CallExpr Implementation
CallExpr::CallExpr(const Token &token, ASTNodePtr callee,
                                 std::vector<ASTNodePtr> arguments)
    : ASTNode(ASTNodeType::CallExpr, token), callee(std::move(callee)),
      arguments(std::move(arguments)) {}

void CallExpr::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // Resolve callee type first
    this->callee->resolveType(ctx);

    // Ensure callee is a function
    const auto type_data =
        ctx.typeRegistry.getTypeData(this->callee->result_type.value());
    if (!std::holds_alternative<FunctionType>(type_data)) {
        throw ParserError(this->token,
                "Callee isn't a function");
    }
    const FunctionType &function_type = std::get<FunctionType>(type_data);

    // The result_type of this call is the return type of the function
    this->result_type = function_type.return_type;

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

        auto cast_result = CastExpr::tryCast(
            std::move(this->arguments[i]),
            function_type.param_types[i],
            ctx);
        if (!cast_result.has_value())
            throw ParserError(
                this->arguments[i]->token,
                std::format("Incompatible type for argument {} in call", i + 1));
        this->arguments[i] = std::move(cast_result.value());
    }

    // Everything ok
}

void CallExpr::compile(CompileContext &ctx, int reg) {
    // Get a register for the result
    this->result_register = reg == -1 ? ctx.allocateRegister() : reg;

    // Get continous registers for the arguments and callee
    std::vector<int> arg_registers;
    for (size_t i = 0; i < this->arguments.size() + 1; i++)
        arg_registers.push_back(ctx.allocateFromTop());

    // Compile the callee
    this->callee->compile(ctx, arg_registers[0]);

    // Compile the arguments
    int i = 1;
    for (auto &arg : this->arguments)
        arg->compile(ctx, arg_registers[i++]);

    // Emit call
    ctx.function->chunk.write_AB(OpCode::Call, this->callee->result_register,
                                 this->arguments.size(), this->token.line);

    // The result value is now in the callee's result register, move it
    ctx.function->chunk.write_AB(
        OpCode::Copy, this->callee->result_register,
        this->result_register, this->token.line);

    // Free the argument registers in reverse order
    for (int j = this->arguments.size(); j >= 0; j--)
        ctx.freeRegister(arg_registers[j]);
}

void CallExpr::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "CallExpr()\n";
    this->callee->print(indent + 1);
    for (auto &arg : this->arguments)
        arg->print(indent + 1);
}


