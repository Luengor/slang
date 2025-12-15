#include "compiler.hpp"
#include "ast.hpp"
#include "scanner.hpp"

Compiler::Compiler(const std::string &source) : source(source) {}

Chunk Compiler::compile() {
    // Scan source
    Scanner scanner;
    std::vector<Token> tokens = scanner.scan(this->source);

    for (const Token &token : tokens) {
        // For now, just print the tokens
        // In a real compiler, you'd convert tokens to bytecode here
        // but this is a placeholder implementation
        printf("Token: %-10s Lexeme: %-15s Line: %d\n",
               std::to_string(token.type).c_str(),
               token.lexeme.c_str(),
               token.line);
    }

    // magic
    std::vector<Token> manual_token_list = {
        {Token::Type::Number, "2", 1},
        {Token::Type::Star, "*", 1},
        {Token::Type::Minus, "-", 1},
        {Token::Type::Number, "3", 1},
    };

    LiteralNode *left_node = new LiteralNode(manual_token_list[0]);
    LiteralNode *right_node = new LiteralNode(manual_token_list[3]);
    UnaryExpressionNode *right_unary =
        new UnaryExpressionNode(manual_token_list[2], right_node);
    BinaryExpressionNode *root =
        new BinaryExpressionNode(manual_token_list[1], left_node, right_unary);

    Chunk chunk;
    root->compile(chunk);
    chunk.write(OpCode::Return, 1);
    chunk.disassemble("Test Chunk");
    return chunk;
}

