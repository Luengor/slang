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

void Chunk::disassembleAb(const char *name, uint32_t instruction) const {
    uint16_t A = GET_Ab_a(instruction);
    uint8_t b = GET_Ab_b(instruction);

    std::println("{:<16} {:4d} {:4d}", name, A, b);
}

void Chunk::disassembleInstruction(int offset) {
    std::print("{:04d} ", offset);

    if (offset > 0 && this->lines[offset - 1] == this->lines[offset])
        std::print("   | ");
    else
        std::print("{:4d} ", this->lines[offset]);

    OpCode instruction = GET_op(this->code[offset]); 
    switch (instruction) {
        case OpCode::Return:
            std::println("OP_RETURN");
            break;

        case OpCode::Constant:
            return this->disassembleAb("OP_CONSTANT", this->code[offset]);

        case OpCode::Call:
            std::println("OP_CALL");
            break;

        case OpCode::NegateF:
            std::println("OP_NEGATEF");
            break;
        case OpCode::NegateI:
            std::println("OP_NEGATEI");
            break;

        case OpCode::AddF:
            std::println("OP_ADDF");
            break;
        case OpCode::AddI:
            std::println("OP_ADDI");
            break;

        case OpCode::SubtractF:
            std::println("OP_SUBTRACTF");
            break;
        case OpCode::SubtractI:
            std::println("OP_SUBTRACTI");
            break;

        case OpCode::MultiplyF:
            std::println("OP_MULTIPLYF");
            break;
        case OpCode::MultiplyI:
            std::println("OP_MULTIPLYI");
            break;

        case OpCode::DivideF:
            std::println("OP_DIVIDEF");
            break;
        case OpCode::DivideI:
            std::println("OP_DIVIDEI");
            break;

        case OpCode::Object:
            std::println("OP_OBJECT");
            break;

        case OpCode::Not:
            std::println("OP_NOT");
            break;

        case OpCode::I2F:
            std::println("OP_I2F");
            break;
        case OpCode::F2I:
            std::println("OP_F2I");
            break;
        case OpCode::I2B:
            std::println("OP_I2B");
            break;
        case OpCode::B2I:
            std::println("OP_B2I");
            break;
        case OpCode::F2B:
            std::println("OP_F2B");
            break;
        case OpCode::B2F:
            std::println("OP_B2F");
            break;
        case OpCode::I2Str:
            std::println("OP_I2STR");
            break;
        case OpCode::F2Str:
            std::println("OP_F2STR");
            break;
        case OpCode::B2Str:
            std::println("OP_B2STR");
            break;

        case OpCode::EqI:
            std::println("OP_EQI");
            break;
        case OpCode::NeI:
            std::println("OP_NEI");
            break;
        case OpCode::EqF:
            std::println("OP_EQF");
            break;
        case OpCode::NeF:
            std::println("OP_NEF");
            break;
        case OpCode::EqB:
            std::println("OP_EQB");
            break;
        case OpCode::NeB:
            std::println("OP_NEB");
            break;

        case OpCode::GtI:
            std::println("OP_GTI");
            break;
        case OpCode::LtI:
            std::println("OP_LTI");
            break;
        case OpCode::GeI:
            std::println("OP_GEI");
            break;
        case OpCode::LeI:
            std::println("OP_LEI");
            break;
        case OpCode::GtF:
            std::println("OP_GTF");
            break;
        case OpCode::LtF:
            std::println("OP_LTF");
            break;
        case OpCode::GeF:
            std::println("OP_GEF");
            break;
        case OpCode::LeF:
            std::println("OP_LEF");
            break;

        case OpCode::True:
            std::println("OP_TRUE");
            break;
        case OpCode::False:
            std::println("OP_FALSE");
            break;
        case OpCode::Pop:
            std::println("OP_POP");
            break;
        case OpCode::Retain:
            std::println("OP_RETAIN");
            break;
        case OpCode::Release:
            std::println("OP_RELEASE");
            break;

        case OpCode::Jmp:
            std::println("OP_JMP");
            break;
        case OpCode::JmpIfFalse:
            std::println("OP_JNT");
            break;
        case OpCode::JmpIfTrue:
            std::println("OP_JIT");
            break;
        case OpCode::JmpIfFalsePop:
            std::println("OP_JNT_POP");
            break;

        case OpCode::Move:
            std::println("OP_MOVE");
            break;
        case OpCode::GetLocal:
            std::println("OP_GETLOCAL");
            break;
        case OpCode::SetLocal:
            std::println("OP_SETLOCAL");
            break;
        case OpCode::GetLocalObject:
            std::println("OP_GETLOCAL_OBJ");
            break;
        case OpCode::SetLocalObject:
            std::println("OP_SETLOCAL_OBJ");
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
        this->disassembleInstruction(i);
    }
}

