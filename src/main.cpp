#include "chunk.hpp"
#include "vm.hpp"

int main() {
    Chunk chunk;
    VM vm;

    // Constant
    const auto constant = chunk.addConstant(3.1415);
    chunk.write(OpCode::Constant, 1);
    chunk.write(constant, 1);

    // Return
    chunk.write(OpCode::Return, 1);

    // dis
    vm.interpret(std::move(chunk));

    return 0;
}
