#include "ast_core.hpp"
#include "native.hpp"
#include "object.hpp"

ASTNode::ASTNode(ASTNodeType type, const Token &token)
    : type(type), token(token) {}

std::unique_ptr<FunctionObj> compileAST(ASTNode *root) {
    // Create compile context
    TypeRegistry typeRegistry;

    NativeRegistry nativeRegistry(typeRegistry);

    std::unique_ptr<FunctionObj> function = std::make_unique<FunctionObj>();
    function->name = "<main>";
    function->type_id = typeRegistry.getFunction({}, typeRegistry.noneType());
    
    CompileContext ctx{
        .function = function.get(), 
        .typeRegistry = typeRegistry,
        .nativeRegistry = nativeRegistry,
        .scope_depth = 0,
        .locals = {},
    };

    // Perform type resolution
    root->resolveType(ctx);

    // Compile the AST
    root->compile(ctx);

    function->chunk.write(OpCode::Return);

    return function;
}
