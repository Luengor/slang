#include "ast_core.hpp"

ASTNode::ASTNode(ASTNodeType type, const Token &token)
    : type(type), token(token) {}

Chunk compileAST(ASTNode *root) {
    // Create compile context
    Chunk chunk;
    TypeRegistry typeRegistry;
    CompileContext ctx{
        .chunk = &chunk,
        .typeRegistry = typeRegistry,

        .scope_depth = 0,
        .locals = {},
    };

    // Perform type resolution
    root->resolveType(ctx);

    // Compile the AST
    root->compile(ctx);

    chunk.write(OpCode::Return);

    return chunk;
}

