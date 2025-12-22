#include "ast.hpp"
#include "chunk.hpp"
#include "error.hpp"
#include "types.hpp"
#include "value.hpp"
#include <iostream>
#include <print>

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

// UnaryExpressionNode Implementation
UnaryExpressionNode::UnaryExpressionNode(const Token &token, ASTNodePtr operand)
    : ASTNode(ASTNodeType::UnaryExpression, token), operand(std::move(operand)) {}

void UnaryExpressionNode::resolveType(CompileContext &ctx) {
    // Resolve the operand type first
    this->operand->resolveType(ctx);
    const auto operand_type = this->operand->result_type;

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
        case Token::Type::Bang:
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

void UnaryExpressionNode::compile(CompileContext &ctx) {
    // Compile the operand first
    this->operand->compile(ctx);

    // Compile the appropriate unary operation
    switch (this->token.type) {
        case Token::Type::Minus:
            if (this->result_type ==
                ctx.typeRegistry.getPrimitive(PrimitiveKind::Fixed)) {
                ctx.chunk.write(OpCode::NegateF, this->token.line);
            } else {
                ctx.chunk.write(OpCode::NegateI, this->token.line);
            }
            break;

        case Token::Type::Not:
        case Token::Type::Bang:
            ctx.chunk.write(OpCode::Not, this->token.line);
            break;

        default:
            throw ParserError(
                this->token,
                "Unsupported unary operator during compilation.");
            break;
    }
}

void UnaryExpressionNode::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "UnaryExpression(" << this->token.lexeme << ")\n";
    this->operand->print(indent + 1);
}

// BinaryExpressionNode Implementation
BinaryExpressionNode::BinaryExpressionNode(const Token &token, ASTNodePtr left,
                                           ASTNodePtr right)
    : ASTNode(ASTNodeType::BinaryExpression, token), left(std::move(left)),
      right(std::move(right)) { }

void BinaryExpressionNode::resolveType(CompileContext &ctx) {
    // Resolve left and right operand types first
    this->left->resolveType(ctx);
    this->right->resolveType(ctx);

    // Check that both operands have the same type
    if (this->left->result_type != this->right->result_type) {
        throw ParserError(
            this->token,
            "Type mismatch between left and right operands in binary expression.");
    } else if (this->left->result_type == ctx.typeRegistry.noneType()) {
        throw ParserError(
            this->token,
            "Binary expressions do not support 'none' type.");
    }

    // Set the result type to the operand type
    this->result_type = this->left->result_type;
}

void BinaryExpressionNode::compile(CompileContext &ctx) {
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

        case Token::Type::And:
        case Token::Type::Or:
            return compileLogical(ctx);

        default:
            throw ParserError(
                this->token,
                "Unsupported binary operator during compilation.");
            break;
    }
}

void BinaryExpressionNode::compileArithmetic(CompileContext &ctx) {
    const TypeID floatingType =
        ctx.typeRegistry.getPrimitive(PrimitiveKind::Floating);

    // Check that the type is numeric
    if (!ctx.typeRegistry.isNumeric(this->result_type)) {
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

void BinaryExpressionNode::compileLogical(CompileContext &ctx) {
    // Check that the type is boolean
    const TypeID booleanType =
        ctx.typeRegistry.getPrimitive(PrimitiveKind::Boolean);
    if (this->result_type != booleanType) {
        throw ParserError(
            this->token,
            "Logical operators require boolean operand types.");
    }

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

void BinaryExpressionNode::print(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "BinaryExpression(" << this->token.lexeme << ")\n";
    this->left->print(indent + 1);
    this->right->print(indent + 1);
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

