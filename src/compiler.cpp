#include "compiler.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "scanner.hpp"

Compiler::Compiler(const std::string &source) : source(source) {}

Chunk Compiler::compile() {
    // Scan source
    Scanner scanner;
    std::vector<Token> tokens = scanner.scan(this->source);

    // Create AST
    Parser parser;
    ASTNode *root = parser.parse(tokens); 

    // Compile AST to Chunk
    Chunk chunk;
    if (root != nullptr) {
        root->compile(chunk);
    }

    // Clean up
    delete root;

    return chunk;
}

