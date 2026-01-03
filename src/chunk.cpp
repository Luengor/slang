#include "chunk.hpp"
#include "object.hpp"
#include <cassert>
#include <print>

Chunk::~Chunk() {
    // Release all object constants
    for (auto object : this->object_constants) {
        object->release();
    }
}

void Chunk::disassebleInstruction(int offset) {
    std::print("{:04d} ", offset);

    if (offset > 0 && this->lines[offset - 1] == this->lines[offset])
        std::print("   | ");
    else
        std::print("{:4d} ", this->lines[offset]);

    OpCode instruction = GET_op(this->code[offset]); 
    switch (instruction) {
        case OpCode::Return:
            std::print("OP_RETURN");
            break;

        case OpCode::Call:
            std::print("OP_CALL");
            break;

        case OpCode::NegateF:
            std::print("OP_NEGATEF");
            break;
        case OpCode::NegateI:
            std::print("OP_NEGATEI");
            break;

        case OpCode::AddF:
            std::print("OP_ADDF");
            break;
        case OpCode::AddI:
            std::print("OP_ADDI");
            break;

        case OpCode::SubtractF:
            std::print("OP_SUBTRACTF");
            break;
        case OpCode::SubtractI:
            std::print("OP_SUBTRACTI");
            break;

        case OpCode::MultiplyF:
            std::print("OP_MULTIPLYF");
            break;
        case OpCode::MultiplyI:
            std::print("OP_MULTIPLYI");
            break;

        case OpCode::DivideF:
            std::print("OP_DIVIDEF");
            break;
        case OpCode::DivideI:
            std::print("OP_DIVIDEI");
            break;

        case OpCode::Constant:
            std::print("OP_CONSTANT");
            break;
        case OpCode::Object:
            std::print("OP_OBJECT");
            break;

        case OpCode::Not:
            std::print("OP_NOT");
            break;

        case OpCode::I2F:
            std::print("OP_I2F");
            break;
        case OpCode::F2I:
            std::print("OP_F2I");
            break;
        case OpCode::I2B:
            std::print("OP_I2B");
            break;
        case OpCode::B2I:
            std::print("OP_B2I");
            break;
        case OpCode::F2B:
            std::print("OP_F2B");
            break;
        case OpCode::B2F:
            std::print("OP_B2F");
            break;
        case OpCode::I2Str:
            std::print("OP_I2STR");
            break;
        case OpCode::F2Str:
            std::print("OP_F2STR");
            break;
        case OpCode::B2Str:
            std::print("OP_B2STR");
            break;

        case OpCode::EqI:
            std::print("OP_EQI");
            break;
        case OpCode::NeI:
            std::print("OP_NEI");
            break;
        case OpCode::EqF:
            std::print("OP_EQF");
            break;
        case OpCode::NeF:
            std::print("OP_NEF");
            break;
        case OpCode::EqB:
            std::print("OP_EQB");
            break;
        case OpCode::NeB:
            std::print("OP_NEB");
            break;

        case OpCode::GtI:
            std::print("OP_GTI");
            break;
        case OpCode::LtI:
            std::print("OP_LTI");
            break;
        case OpCode::GeI:
            std::print("OP_GEI");
            break;
        case OpCode::LeI:
            std::print("OP_LEI");
            break;
        case OpCode::GtF:
            std::print("OP_GTF");
            break;
        case OpCode::LtF:
            std::print("OP_LTF");
            break;
        case OpCode::GeF:
            std::print("OP_GEF");
            break;
        case OpCode::LeF:
            std::print("OP_LEF");
            break;

        case OpCode::True:
            std::print("OP_TRUE");
            break;
        case OpCode::False:
            std::print("OP_FALSE");
            break;
        case OpCode::Pop:
            std::print("OP_POP");
            break;
        case OpCode::Retain:
            std::print("OP_RETAIN");
            break;
        case OpCode::Release:
            std::print("OP_RELEASE");
            break;

        case OpCode::Jmp:
            std::print("OP_JMP");
            break;
        case OpCode::JmpIfFalse:
            std::print("OP_JNT");
            break;
        case OpCode::JmpIfTrue:
            std::print("OP_JIT");
            break;
        case OpCode::JmpIfFalsePop:
            std::print("OP_JNT_POP");
            break;

        case OpCode::Move:
            std::print("OP_MOVE");
            break;
        case OpCode::GetLocal:
            std::print("OP_GETLOCAL");
            break;
        case OpCode::SetLocal:
            std::print("OP_SETLOCAL");
            break;
        case OpCode::GetLocalObject:
            std::print("OP_GETLOCAL_OBJ");
            break;
        case OpCode::SetLocalObject:
            std::print("OP_SETLOCAL_OBJ");
            break;

        default:
            std::print("Unknown opcode {}\n",
                       static_cast<uint8_t>(instruction));
            break;
    }
}

#define FIX_LINE line = line == -1 ? this->lines.empty() ? 0 : this->lines.back() : line;

// abc -> [OpCode:8] [A:8] [B:8] [C:8]
unsigned Chunk::write_abc(OpCode op, uint8_t a, uint8_t b, uint8_t c, int line) {
    FIX_LINE;
    uint32_t instruction = (static_cast<uint8_t>(op) << 24) |
                           (a << 16) | (b << 8) | c;
    this->code.push_back(instruction);
    this->lines.push_back(line);
    return this->code.size() - 1;
}

// Sa -> [OpCode:8] [Unused:8] [A:16]
unsigned Chunk::write_sA(OpCode op, int16_t A, int line) {
    FIX_LINE;
    uint32_t instruction = (static_cast<uint8_t>(op) << 24) |
                           (static_cast<uint16_t>(A) & 0xFFFF);
    this->code.push_back(instruction);
    this->lines.push_back(line);
    return this->code.size() - 1;
}

unsigned Chunk::write_A(OpCode op, uint16_t A, int line) {
    FIX_LINE;
    uint32_t instruction = (static_cast<uint8_t>(op) << 24) |
                           (static_cast<uint16_t>(A) & 0xFFFF);
    this->code.push_back(instruction);
    this->lines.push_back(line);
    return this->code.size() - 1;
}

// A and B are both 12 bits
unsigned Chunk::write_AB(OpCode op, uint16_t A, uint16_t B, int line) {
    FIX_LINE;
    uint32_t instruction = (static_cast<uint8_t>(op) << 24) |
                           ((A & 0x0FFF) << 12) | (B & 0x0FFF);
    this->code.push_back(instruction);
    this->lines.push_back(line);
    return this->code.size() - 1;
}

unsigned Chunk::write_Ab(OpCode op, uint16_t A, uint8_t b, int line) {
    FIX_LINE;
    uint32_t instruction = (static_cast<uint8_t>(op) << 24) |
                           ((A & 0x0FFFF) << 8) | (b & 0x00FF);
    this->code.push_back(instruction);
    this->lines.push_back(line);
    return this->code.size() - 1;
}

void Chunk::patch_sA(unsigned offset, int16_t A) {
    assert(offset < this->code.size());
    uint32_t instruction = this->code[offset];
    instruction = (instruction & 0xFFFF0000) | (static_cast<uint16_t>(A) & 0xFFFF);
    this->code[offset] = instruction;
}

unsigned Chunk::currentOffset() const {
    return this->code.size();
}

int Chunk::addConstant(Value value) {
    this->constants.push_back(value);
    return this->constants.size() - 1;
}

int Chunk::addObjectConstant(Object *obj) {
    this->object_constants.push_back(obj);
    return this->object_constants.size() - 1;
}

void Chunk::disassemble(const std::string &header) {
    std::print("== {} ==\n", header);

    for (uint i = 0; i < this->code.size(); i++) {
        this->disassebleInstruction(i);
    }
}

