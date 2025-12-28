#include "ast.hpp"
#include "chunk.hpp"
#include "error.hpp"
#include "types.hpp"
#include "value.hpp"
#include <iostream>
#include <print>
#include <variant>

// overload boilerplate
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

#define ResolveGuard \
    if (this->result_type.has_value()) { \
        return; \
    }

// CompileContext
int CompileContext::addLocal(const std::string &name, TypeID type) {
    // Check if there are any existing locals with the same name
    // defined in the same scope
    for (auto it = this->locals.rbegin(); it != this->locals.rend(); ++it) {
        if (it->name == name && it->depth == this->scope_depth) {
            return -1; // Indicate error: duplicate local
        }
    }

    this->locals.push_back(
        Local{.name = name, .type = type, .depth = this->scope_depth});
    return static_cast<int>(this->locals.size() - 1);
}

int CompileContext::findLocal(const std::string &name) {
    // Search for the local variable in reverse order (most recent first)
    for (auto it = this->locals.rbegin(); it != this->locals.rend(); ++it) {
        if (it->name == name) {
            return std::distance(it, this->locals.rend()) - 1;
        }
    }

    return -1; // Not found
}

void CompileContext::enterScope() {
    this->scope_depth++;
}

int CompileContext::exitScope() {
    this->scope_depth--;

    int pop_count = 0;
    for (auto it = this->locals.rbegin();
         it != this->locals.rend() && it->depth > this->scope_depth; ++it) {
        // Remove local variables from this scope and count how many to pop
        this->locals.pop_back();
        pop_count++;
    }

    return pop_count;
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
            StringObj *strObj = new StringObj();
            strObj->type = Object::Type::String;
            strObj->value = strValue;

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

void LiteralNode::compile(CompileContext &ctx) {
    const auto constant = ctx.chunk.addConstant(this->value.second);
    ctx.chunk.write(OpCode::Constant, this->token.line);
    ctx.chunk.write(static_cast<uint8_t>(constant), this->token.line);
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
    }
}

// Variable Node Implementation
VariableNode::VariableNode(const Token &token, const std::string &name)
    : ASTNode(ASTNodeType::Variable, token), name(name) {}

// Resolve type is only called on variable use
void VariableNode::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // Find the variable in the local scope
    const int local_index = ctx.findLocal(this->name);
    if (local_index == -1) {
        throw ParserError(this->token,
                          "Undefined variable during type resolution.");
    }

    this->local_index = local_index;
    this->result_type = ctx.locals[local_index].type;
}

void VariableNode::compile(CompileContext &ctx) {
    if (this->local_index == -1) {
        throw ParserError(this->token,
                          "Variable not resolved before compilation.");
    }

    // Load the variable from the local slot
    if (this->local_index > 255) {
        ctx.chunk.write(OpCode::GetLocalLong, this->token.line);
        ctx.chunk.writeWord(static_cast<uint16_t>(this->local_index),
                            this->token.line);
    } else {
        ctx.chunk.write(OpCode::GetLocal, this->token.line);
        ctx.chunk.write(static_cast<uint8_t>(this->local_index),
                        this->token.line);
    }
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

void UnaryExpr::compile(CompileContext &ctx) {
    // Compile the operand first
    this->operand->compile(ctx);

    // Compile the appropriate unary operation
    switch (this->token.type) {
        case Token::Type::Minus:
            if (this->result_type ==
                ctx.typeRegistry.getPrimitive(PrimitiveKind::Fixed)) {
                ctx.chunk.write(OpCode::NegateI, this->token.line);
            } else {
                ctx.chunk.write(OpCode::NegateF, this->token.line);
            }
            break;

        case Token::Type::Not:
            ctx.chunk.write(OpCode::Not, this->token.line);
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
      operand(std::move(operand)) {
    // Copy result type
    this->result_type = target_type;
}

void CastExpr::resolveType(CompileContext &ctx) {
    ResolveGuard;

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

void CastExpr::compile(CompileContext &ctx) {
    // Compile the operand first
    this->operand->compile(ctx);

    // Write the cast operation
    ctx.chunk.write(this->cast_op, this->token.line);
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

void BinaryExpr::compile(CompileContext &ctx) {
    // Compile left and right operands first
    this->left->compile(ctx);
    this->right->compile(ctx);

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
            ctx.chunk.write(IorF(Add), this->token.line);
            break;
        case Token::Type::Minus:
            ctx.chunk.write(IorF(Subtract), this->token.line);
            break;
        case Token::Type::Star:
            ctx.chunk.write(IorF(Multiply), this->token.line);
            break;
        case Token::Type::Slash:
            ctx.chunk.write(IorF(Divide), this->token.line);
            break;

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
            ctx.chunk.write(EqOrNe(I), this->token.line);
            break;

        case PrimitiveKind::Floating:
            ctx.chunk.write(EqOrNe(F), this->token.line);
            break;

        case PrimitiveKind::Boolean:
            ctx.chunk.write(EqOrNe(B), this->token.line);
            break;

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
            ctx.chunk.write(IorF(Gt), this->token.line);
            break;
        case Token::Type::GreaterEqual:
            ctx.chunk.write(IorF(Ge), this->token.line);
            break;
        case Token::Type::Less:
            ctx.chunk.write(IorF(Lt), this->token.line);
            break;
        case Token::Type::LessEqual:
            ctx.chunk.write(IorF(Le), this->token.line);;
            break;
        default:
            throw ParserError(
                this->token,
                "Unsupported comparison operator during compilation.");
            break;
    }
#undef IorF
}

void BinaryExpr::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "BinaryExpr(" << this->token.lexeme << ")\n";
    this->left->print(indent + 1);
    this->right->print(indent + 1);
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

void LogicExpr::compile(CompileContext &ctx) {
    // Compile left and right operands first
    this->left->compile(ctx);
    this->right->compile(ctx);

    // Compile the appropriate logical operation
    switch (this->token.type) {
        case Token::Type::And:
            ctx.chunk.write(OpCode::And, this->token.line);
            break;
        case Token::Type::Or:
            ctx.chunk.write(OpCode::Or, this->token.line);
            break;

        default:
            throw ParserError(
                this->token, "Unsupported logical operator during compilation.");
            break;
    }
}

void LogicExpr::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "LogicExpr(" << this->token.lexeme << ")\n";
    this->left->print(indent + 1);
    this->right->print(indent + 1);
}

// ExprStmt Implementation
ExprStmt::ExprStmt(const Token &token, ASTNodePtr expression)
    : ASTNode(ASTNodeType::ExprStmt, token),
      expression(std::move(expression)) {}

void ExprStmt::resolveType(CompileContext &ctx) {
    // Resolve the expression type
    this->expression->resolveType(ctx);

    // Expression statements have no result type
    this->result_type = ctx.typeRegistry.noneType();
}

void ExprStmt::compile(CompileContext &ctx) {
    // Compile the expression
    this->expression->compile(ctx);

    // Pop the result off the stack since it's not used
    ctx.chunk.write(OpCode::Pop, this->token.line);
}

void ExprStmt::print(int indent) {
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
        ctx.chunk.write(OpCode::Pop, this->token.line);
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
VarDeclStmt::VarDeclStmt(const Token &type_token, const Token &name_token,
                         ASTNodePtr initializer)
    : ASTNode(ASTNodeType::VarDeclStmt, name_token),
      initializer(std::move(initializer)), type_token(type_token) {}

void VarDeclStmt::resolveType(CompileContext &ctx) {
    ResolveGuard;

    // If there's an initializer, resolve its type
    if (this->initializer) {
        this->initializer->resolveType(ctx);
    }

    // If the variable type is auto, infer from initializer
    if (this->type_token.type == Token::Type::Auto) {
        if (!this->initializer) {
            throw ParserError(
                this->token,
                "Auto variable declaration requires an initializer.");
        }

        this->result_type = this->initializer->result_type;
    }

    // If no initializer, just use the declared type
    else {
        switch (this->type_token.type) {
            case Token::Type::Fixed:
                this->result_type =
                    ctx.typeRegistry.getPrimitive(PrimitiveKind::Fixed);
                break;

            case Token::Type::Float:
                this->result_type =
                    ctx.typeRegistry.getPrimitive(PrimitiveKind::Floating);
                break;

            case Token::Type::Bool:
                this->result_type =
                    ctx.typeRegistry.getPrimitive(PrimitiveKind::Boolean);
                break;

            default:
                throw ParserError(this->token,
                                  "Unsupported variable type in declaration.");
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
        const auto constant = ctx.chunk.addConstant(defaultValue);
        ctx.chunk.write(OpCode::Constant, this->token.line);
        ctx.chunk.write(static_cast<uint8_t>(constant), this->token.line);
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
        ctx.chunk.write(OpCode::SetLocalLong, this->token.line);
        ctx.chunk.writeWord(static_cast<uint16_t>(varNode->local_index),
                            this->token.line);
    } else {
        ctx.chunk.write(OpCode::SetLocal, this->token.line);
        ctx.chunk.write(static_cast<uint8_t>(varNode->local_index),
                        this->token.line);
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
    ctx.chunk.write(OpCode::JmpIfFalsePop, this->token.line);
    const auto if_jump =
        ctx.chunk.writeWord(0xFFFF, this->token.line); // Placeholder

    // Compile then branch
    this->then_branch->compile(ctx);
    
    // If there is an else branch, insert jump to after else
    unsigned else_jump = 0;
    if (this->else_branch) {
        ctx.chunk.write(OpCode::Jmp, this->token.line);
        else_jump =
            ctx.chunk.writeWord(0xFFFF, this->token.line); // Placeholder
    }

    // Patch first jump
    const unsigned after_then_addr = ctx.chunk.currentOffset();
    const int16_t offset_to_after_then =
        static_cast<int16_t>(after_then_addr - (if_jump + 2));
    ctx.chunk.patchWord(if_jump, offset_to_after_then);

    // If there is no else branch, we're done
    if (!this->else_branch)
        return;

    // Compile else branch
    this->else_branch->compile(ctx);

    // Patch else jump
    const unsigned after_else_addr = ctx.chunk.currentOffset();
    const int16_t offset_to_after_else =
        static_cast<int16_t>(after_else_addr - (else_jump + 2));
    ctx.chunk.patchWord(else_jump, offset_to_after_else);
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

Chunk compileAST(ASTNode *root) {
    // Create compile context
    Chunk chunk;
    TypeRegistry typeRegistry;
    CompileContext ctx{
        .chunk = chunk,
        .typeRegistry = typeRegistry,

        .scope_depth = 0,
        .locals = {},
    };

    // Perform type resolution
    root->resolveType(ctx);

    // Compile the AST
    root->compile(ctx);

    chunk.write(OpCode::Return, 0);

    return chunk;
}

