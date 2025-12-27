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

    // Resolve types for all statements
    for (auto &stmt : this->statements) {
        stmt->resolveType(ctx);
    }

    // For now, block statements have no result type
    // Maybe in the future we can have the last statement's type?
    this->result_type = ctx.typeRegistry.noneType();
}

void BlockStmt::compile(CompileContext &ctx) {
    // Compile all statements in the block
    for (auto &stmt : this->statements) {
        stmt->compile(ctx);
    }
}

void BlockStmt::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "BlockStmt\n";
    for (auto &stmt : this->statements) {
        stmt->print(indent + 1);
    }
}

Chunk compileAST(ASTNode *root) {
    // Create compile context
    Chunk chunk;
    TypeRegistry typeRegistry;
    CompileContext ctx{chunk, typeRegistry};

    // Perform type resolution
    root->resolveType(ctx);

    // Compile the AST
    root->compile(ctx);

    chunk.write(OpCode::Return, 0);

    return chunk;
}

