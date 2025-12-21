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
    if (token.type == Token::Type::Number) {
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
    } else if (token.type == Token::Type::String) {
        // It's a string literal
        // Remove the surrounding quotes
        std::string strValue = token.lexeme.substr(1, token.lexeme.length() - 2);

        // Create a StringObj
        StringObj *strObj = new StringObj();
        strObj->type = Object::Type::String;
        strObj->value = strValue;

        Value val {.object = strObj};
        this->value = std::make_pair(ValueType::Object, val);
    } else {
        throw ParserError(token, "Unsupported literal type.");
    }
}

CompileResult LiteralNode::compile(CompileContext &ctx) {
    const auto constant = ctx.chunk.addConstant(this->value.second);
    ctx.chunk.write(OpCode::Constant, this->token.line);
    ctx.chunk.write(static_cast<uint8_t>(constant), this->token.line);

    // Get result type
    TypeID result_type = ctx.typeRegistry.getFromValue(this->value);
    if (result_type == ctx.typeRegistry.noneType()) {
        throw ParserError(this->token,
                          "Unknown literal type during compilation.");
    }

    return {result_type};
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

CompileResult UnaryExpressionNode::compile(CompileContext &ctx) {
    const auto operand_result = operand->compile(ctx);

    // Type check
    const auto fixedType = ctx.typeRegistry.getPrimitive(PrimitiveKind::Fixed);
    const auto floatingType = ctx.typeRegistry.getPrimitive(PrimitiveKind::Floating);

    switch (this->token.type) {
        case Token::Type::Minus:
            if (operand_result.result_type == floatingType) {
                ctx.chunk.write(OpCode::NegateF, this->token.line);
            } else if (operand_result.result_type == fixedType) {
                ctx.chunk.write(OpCode::NegateI, this->token.line);
            } else {
                throw ParserError(
                    this->token,
                    "Unary minus operator requires a numeric operand.");
            }
            break;

        default:
            throw ParserError(
                this->token,
                "Unsupported unary operator during compilation.");
            break;
    }

    return {operand_result.result_type};
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

CompileResult BinaryExpressionNode::compile(CompileContext &ctx) {
    const auto lresult = left->compile(ctx);
    const auto rresult = right->compile(ctx);

    // For now, check that both operands have the same type
    if (lresult.result_type != rresult.result_type) {
        throw ParserError(
            this->token,
            "Type mismatch between left and right operands in binary expression.");
    }

    // Also check that the type is either Fixed or Floating
    const TypeID fixedType = ctx.typeRegistry.getPrimitive(PrimitiveKind::Fixed);
    const TypeID floatingType = ctx.typeRegistry.getPrimitive(PrimitiveKind::Floating);
    if (lresult.result_type != fixedType && lresult.result_type != floatingType) {
        throw ParserError(
            this->token,
            "Binary expressions only support Fixed and Floating point types.");
    }

    // Compile
#define IorF(op) lresult.result_type == floatingType ? OpCode::op##F : OpCode::op##I

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
                this->token,
                "Unsupported binary operator during compilation.");
            break;
    }

#undef IorF

    return {lresult.result_type};
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

    // Compile the AST
    root->compile(ctx);

    chunk.write(OpCode::Return, 0);

    return chunk;
}

