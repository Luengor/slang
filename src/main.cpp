#include "chunk.hpp"

int main() {
    Chunk chunk;

    // Constant
    const auto constant = chunk.addConstant(3.1415);
    chunk.write(OpCode::Constant, 1);
    chunk.write(constant, 1);

    // Return
    chunk.write(OpCode::Return, 1);

    // dis
    chunk.disassemble("test chunk");

    return 0;
}
