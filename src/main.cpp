#include "chunk.hpp"
#include "vm.hpp"

int main() {
    Chunk chunk;
    VM vm;

    // Calculate circonference of the earth 
    const auto pi = chunk.addConstant(3.1415);
    const auto radius = chunk.addConstant(6371.0);
    const auto two = chunk.addConstant(2.0);

    // 2 * pi
    chunk.write(OpCode::Constant, 1);
    chunk.write(static_cast<uint8_t>(two), 1);
    chunk.write(OpCode::Constant, 1);
    chunk.write(static_cast<uint8_t>(pi), 1);
    chunk.write(OpCode::Multiply, 1);

    // radius * (2 * pi)
    chunk.write(OpCode::Constant, 2);
    chunk.write(static_cast<uint8_t>(radius), 2);
    chunk.write(OpCode::Multiply, 2);

    // Return
    chunk.write(OpCode::Return, 1);

    // dis
    vm.interpret(std::move(chunk));

    return 0;
}
