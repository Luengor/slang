#include "chunk.hpp"
#include <cassert>
#include <print>

int Chunk::simpleInstruction(const char *name, int offset) {
    std::print("{}\n", name);
    return offset+1;
}

int Chunk::constantInstruction(const char *name, int offset) {
    const auto constant_i = this->code[offset + 1];
    const auto constant = this->constants[constant_i];
    std::print("{} {:4d} '{:g}'/'{:d}' {}\n", name, constant_i, constant.floating, constant.fixed, constant.boolean);
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

        case OpCode::NegateF: return simpleInstruction("OP_NEGATEF", offset);
        case OpCode::NegateI: return simpleInstruction("OP_NEGATEI", offset);

        case OpCode::AddF: return simpleInstruction("OP_ADDF", offset);
        case OpCode::AddI: return simpleInstruction("OP_ADDI", offset);

        case OpCode::SubtractF: return simpleInstruction("OP_SUBTRACTF", offset);
        case OpCode::SubtractI: return simpleInstruction("OP_SUBTRACTI", offset);

        case OpCode::MultiplyF: return simpleInstruction("OP_MULTIPLYF", offset);
        case OpCode::MultiplyI: return simpleInstruction("OP_MULTIPLYI", offset);

        case OpCode::DivideF: return simpleInstruction("OP_DIVIDEF", offset);
        case OpCode::DivideI: return simpleInstruction("OP_DIVIDEI", offset);

        case OpCode::Constant: return constantInstruction("OP_CONSTANT", offset);

        case OpCode::Not: return simpleInstruction("OP_NOT", offset);
        case OpCode::And: return simpleInstruction("OP_AND", offset);
        case OpCode::Or: return simpleInstruction("OP_OR", offset);

        case OpCode::I2F: return simpleInstruction("OP_I2F", offset);
        case OpCode::F2I: return simpleInstruction("OP_F2I", offset);
        case OpCode::I2B: return simpleInstruction("OP_I2B", offset);
        case OpCode::B2I: return simpleInstruction("OP_B2I", offset);
        case OpCode::F2B: return simpleInstruction("OP_F2B", offset);
        case OpCode::B2F: return simpleInstruction("OP_B2F", offset);

        case OpCode::EqI: return simpleInstruction("OP_EQI", offset);
        case OpCode::NeI: return simpleInstruction("OP_NEI", offset);
        case OpCode::EqF: return simpleInstruction("OP_EQF", offset);
        case OpCode::NeF: return simpleInstruction("OP_NEF", offset);
        case OpCode::EqB: return simpleInstruction("OP_EQB", offset);
        case OpCode::NeB: return simpleInstruction("OP_NEB", offset);

        case OpCode::GtI: return simpleInstruction("OP_GTI", offset);
        case OpCode::LtI: return simpleInstruction("OP_LTI", offset);
        case OpCode::GeI: return simpleInstruction("OP_GEI", offset);
        case OpCode::LeI: return simpleInstruction("OP_LEI", offset);
        case OpCode::GtF: return simpleInstruction("OP_GTF", offset);
        case OpCode::LtF: return simpleInstruction("OP_LTF", offset);
        case OpCode::GeF: return simpleInstruction("OP_GEF", offset);
        case OpCode::LeF: return simpleInstruction("OP_LEF", offset);

        case OpCode::Pop: return simpleInstruction("OP_POP", offset);

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

