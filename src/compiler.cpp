#include "compiler.hpp"
#include "ast_core.hpp"
#include "parser.hpp"
#include "scanner.hpp"
#include "object.hpp"
#include <memory>
#include <stdexcept>

#ifdef DEBUG_PRINT
#include <iostream>
#endif

Compiler::Compiler(const std::string &source) : source(source) {}

std::unique_ptr<FunctionObj> Compiler::compile() {
    // Scan source
    Scanner scanner;
    std::vector<Token> tokens = scanner.scan(this->source);

    // Create AST
    Parser parser(tokens);
    std::unique_ptr<ASTNode> root = parser.parse();

    // Compile AST to Chunk
    if (root == nullptr)
        throw std::runtime_error("Parsing failed.");

    auto function = compileAST(root.get());

#ifdef DEBUG_PRINT
    // Print AST for debugging _after_ compilation
    // (because compilation may modify the AST, e.g., inserting casts)
    std::cout << "\nAST:\n";
    if (root)
        root->print();
    else
        std::cout << "No AST generated.\n";
#endif


#ifdef DEBUG_PRINT
    // Print Chunk for debugging
    std::cout << "\nCompiled Chunk:\n";
    function->chunk.disassemble("code");
#endif

    return function;
}

