#include "chunk.hpp"

int main() {
    Chunk chunk;
    chunk.write(OpCode::Return);
    chunk.disassemble("test chunk");

    return 0;
}
