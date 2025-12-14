#include "chunk.hpp"
#include <cassert>
#include <print>

static int simpleInstruction(const char *name, int offset) {
    std::print("{}\n", name);
    return offset+1;
}

int Chunk::disassebleInstruction(int offset) {
    std::print("{:04d} ", offset);

    OpCode instruction = static_cast<OpCode>(this->code[offset]);
    switch (instruction) {
        case OpCode::Return:
            return simpleInstruction("OP_RETURN", offset);

        default:
            std::print("Unknown opcode {}\n",
                       static_cast<uint8_t>(instruction));
            return offset + 1;
    }
}

void Chunk::write(uint8_t byte) {
    this->code.push_back(byte);
}

void Chunk::disassemble(const std::string &header) {
    std::print("== {} ==\n", header);

    for (uint i = 0; i < this->code.size(); ) {
        i = disassebleInstruction(i);
    }
}

void Chunk::free() {
    this->code.clear();
}

