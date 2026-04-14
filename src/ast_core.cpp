#include "ast_core.hpp"
#include "native.hpp"
#include "object.hpp"
#include <cassert>

#include <tracy/Tracy.hpp>

ASTNode::ASTNode(ASTNodeType type, const Token &token)
    : type(type), token(token) {}

std::unique_ptr<FunctionObj> compileAST(ASTNode *root) {
    ZoneScopedN("compileAST");

    // Create compile context
    TypeRegistry typeRegistry;

    NativeRegistry nativeRegistry(typeRegistry);

    std::unique_ptr<FunctionObj> function = std::make_unique<FunctionObj>();
    function->name = "<main>";
    function->type_id = typeRegistry.getFunction({}, typeRegistry.noneType());

    // Add itself as first object of the chunk
    function->chunk.addObjectConstant(function.get());

    CompileContext ctx(typeRegistry, nativeRegistry);
    ctx.function = function.get();

    // Perform type resolution
    root->resolveType(ctx);

    // There should be nothing in the current names
    assert(ctx.nameTable.getNamesInScope().empty());

    // Compile the AST
    root->compile(ctx);

    function->chunk.writeABx(OpCode::Return, 0, 0, 0);

#ifdef DEBUG_PRINT
    // Print the name table
    ctx.nameTable.printTable();
#endif

    return function;
}
