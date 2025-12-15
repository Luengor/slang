#include "compiler.hpp"
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

    exit(0);
}

