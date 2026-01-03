#include "ast_core.hpp"
#include "native.hpp"
#include "object.hpp"

ASTNode::ASTNode(ASTNodeType type, const Token &token)
    : type(type), token(token) {}

void ASTNode::resolveNames(CompileContext &_) {
    // Default implementation does nothing
}

std::unique_ptr<FunctionObj> compileAST(ASTNode *root) {
    // Create compile context
    TypeRegistry typeRegistry;

    NativeRegistry nativeRegistry(typeRegistry);

    std::unique_ptr<FunctionObj> function = std::make_unique<FunctionObj>();
    function->name = "<main>";
    function->type_id = typeRegistry.getFunction({}, typeRegistry.noneType());
    
    CompileContext ctx(typeRegistry, nativeRegistry);
    ctx.function = function.get();

    // Resolve names
    root->resolveNames(ctx);

    // Perform type resolution
    root->resolveType(ctx);

    // Compile the AST
    root->compile(ctx);

    function->chunk.write_abc(OpCode::Return, 0, 0, 0, root->token.line);

    return function;
}
