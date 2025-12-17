#include "chunk.hpp"
#include <cassert>
#include <print>

int Chunk::simpleInstruction(const char *name, int offset) {
    std::print("{}\n", name);
    return offset+1;
}

int Chunk::constantInstruction(const char *name, int offset) {
    const auto constant_i = this->code[offset + 1];
    std::print("{} {:4d} '{:g}'\n", name, constant_i, this->constants[constant_i].floating);
    return offset+2;
}

int Chunk::disassebleInstruction(int offset) {
    std::print("{:04d} ", offset);

    if (offset > 0 && this->lines[offset - 1] == this->lines[offset])
        std::print("   | ");
    else
        std::print("{:4d} ", this->lines[offset]);

    OpCode instruction = static_cast<OpCode>(this->code[offset]);
    switch (instruction) {
        case OpCode::Return:
            return simpleInstruction("OP_RETURN", offset);

        case OpCode::Negate:
            return simpleInstruction("OP_NEGATE", offset);

        case OpCode::Add:
            return simpleInstruction("OP_ADD", offset);

        case OpCode::Subtract:
            return simpleInstruction("OP_SUBTRACT", offset);

        case OpCode::Multiply:
            return simpleInstruction("OP_MULTIPLY", offset);

        case OpCode::Divide:
            return simpleInstruction("OP_DIVIDE", offset);

        case OpCode::Constant:
            return constantInstruction("OP_CONSTANT", offset);

        default:
            std::print("Unknown opcode {}\n",
                       static_cast<uint8_t>(instruction));
            return offset + 1;
    }
}

void Chunk::write(uint8_t byte, int line) {
    this->code.push_back(byte);
    this->lines.push_back(line);
}

int Chunk::addConstant(Value value) {
    this->constants.push_back(value);
    return this->constants.size() - 1;
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

