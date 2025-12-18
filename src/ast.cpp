#include "ast.hpp"
#include "chunk.hpp"
#include "error.hpp"
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

    this->resultType = this->value.first;
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
            std::println("Literal({})", this->value.second.fixed);
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
    : ASTNode(ASTNodeType::UnaryExpression, token), operand(std::move(operand)) {
    this->resultType = this->operand->resultType;
}

void UnaryExpressionNode::compile(CompileContext &ctx) {
    operand->compile(ctx);

    switch (this->token.type) {
        case Token::Type::Minus:
            if (this->resultType == ValueType::Floating) {
                ctx.chunk.write(OpCode::NegateF, this->token.line);
            } else if (this->resultType == ValueType::Fixed) {
                ctx.chunk.write(OpCode::NegateI, this->token.line);
            }
            break;

        default:
            // Handle error: unsupported unary operator
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
      right(std::move(right)) {
    // Both operands should have the same type for now and neither can be an Object
    this->resultType = this->left->resultType;
    if (this->resultType == ValueType::Object)
        throw ParserError(
            this->token,
            "Binary expressions with object types are not supported.");

    if (this->right->resultType != this->left->resultType) {
        throw ParserError(
            this->token,
            "Type mismatch between left and right operands in binary expression.");
    }
}

void BinaryExpressionNode::compile(CompileContext &ctx) {
    left->compile(ctx);
    right->compile(ctx);

#define IorF(op) this->resultType == ValueType::Floating ? OpCode::op##F : OpCode::op##I

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
            // Handle error: unsupported binary operator
            break;
    }

#undef IorF
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
    CompileContext ctx{chunk};

    // Compile the AST
    root->compile(ctx);

    chunk.write(OpCode::Return, 0);

    return chunk;
}

