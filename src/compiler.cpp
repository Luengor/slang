#include "compiler.hpp"
#include "ast_core.hpp"
#include "parser.hpp"
#include "scanner.hpp"
#include "object.hpp"
#include <format>
#include <iostream>
#include <memory>
#include <stdexcept>

Compiler::Compiler(const std::string &source) : source(source) {}

std::unique_ptr<FunctionObj> Compiler::compile() {
    // Scan source
    Scanner scanner;
    std::vector<Token> tokens = scanner.scan(this->source);

    // Print tokens for debugging
#ifndef NDEBUG
    std::cout << "Tokens:\n";
    for (const Token &token : tokens) {
        std::cout << std::format("[line {}] Type: {}, Lexeme: '{}'\n",
                                 token.line,
                                 static_cast<int>(token.type),
                                 token.lexeme);
    }
#endif

    // Create AST
    Parser parser(tokens);
    std::unique_ptr<ASTNode> root = parser.parse(); 

    // Compile AST to Chunk
    if (root == nullptr)
        throw std::runtime_error("Parsing failed.");

    auto function = compileAST(root.get());

#ifndef NDEBUG
    // Print AST for debugging _after_ compilation
    // (because compilation may modify the AST, e.g., inserting casts)
    std::cout << "\nAST:\n";
    if (root)
        root->print();
    else
        std::cout << "No AST generated.\n";
#endif


#ifndef NDEBUG
    // Print Chunk for debugging
    std::cout << "\nCompiled Chunk:\n";
    function->chunk.disassemble("code");
#endif

    return function;
}

