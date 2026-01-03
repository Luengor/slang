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

void Chunk::disassembleAB(const char *name, uint32_t instruction) const {
    uint16_t A = GET_AB_a(instruction);
    uint8_t B = GET_AB_b(instruction);

    std::println("{:<16} {:4d} {:4d}", name, A, B);
}

void Chunk::disassembleAb(const char *name, uint32_t instruction) const {
    uint16_t A = GET_Ab_a(instruction);
    uint8_t b = GET_Ab_b(instruction);

    std::println("{:<16} {:4d} {:4d}", name, A, b);
}

void Chunk::disassembleabc(const char *name, uint32_t instruction) const {
    uint8_t a = GET_abc_a(instruction);
    uint8_t b = GET_abc_b(instruction);
    uint8_t c = GET_abc_c(instruction);

    std::println("{:<16} {:4d} {:4d} {:4d}", name, a, b, c);
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

        case OpCode::Not:
            return this->disassembleAB("OP_NOT", this->code[offset]);

        case OpCode::NegateI:
            return this->disassembleAB("OP_NEGATE_I", this->code[offset]);

        case OpCode::NegateF:
            return this->disassembleAB("OP_NEGATE_F", this->code[offset]);

        case OpCode::AddF:
            return this->disassembleabc("OP_ADDF", this->code[offset]);

        case OpCode::AddI:
            return this->disassembleabc("OP_ADDI", this->code[offset]);


        case OpCode::SubtractF:
            return this->disassembleabc("OP_SUBTRACTF", this->code[offset]);

        case OpCode::SubtractI:
            return this->disassembleabc("OP_SUBTRACTI", this->code[offset]);


        case OpCode::MultiplyF:
            return this->disassembleabc("OP_MULTIPLYF", this->code[offset]);

        case OpCode::MultiplyI:
            return this->disassembleabc("OP_MULTIPLYI", this->code[offset]);


        case OpCode::DivideF:
            return this->disassembleabc("OP_DIVIDEF", this->code[offset]);

        case OpCode::DivideI:
            return this->disassembleabc("OP_DIVIDEI", this->code[offset]);


        case OpCode::EqI:
            return this->disassembleabc("OP_EQI", this->code[offset]);

        case OpCode::NeI:
            return this->disassembleabc("OP_NEI", this->code[offset]);

        case OpCode::EqF:
            return this->disassembleabc("OP_EQF", this->code[offset]);

        case OpCode::NeF:
            return this->disassembleabc("OP_NEF", this->code[offset]);

        case OpCode::EqB:
            return this->disassembleabc("OP_EQB", this->code[offset]);

        case OpCode::NeB:
            return this->disassembleabc("OP_NEB", this->code[offset]);


        case OpCode::GtI:
            return this->disassembleabc("OP_GTI", this->code[offset]);

        case OpCode::LtI:
            return this->disassembleabc("OP_LTI", this->code[offset]);

        case OpCode::GeI:
            return this->disassembleabc("OP_GEI", this->code[offset]);

        case OpCode::LeI:
            return this->disassembleabc("OP_LEI", this->code[offset]);

        case OpCode::GtF:
            return this->disassembleabc("OP_GTF", this->code[offset]);

        case OpCode::LtF:
            return this->disassembleabc("OP_LTF", this->code[offset]);

        case OpCode::GeF:
            return this->disassembleabc("OP_GEF", this->code[offset]);

        case OpCode::LeF:
            return this->disassembleabc("OP_LEF", this->code[offset]);

        case OpCode::Copy:
            return this->disassembleAB("OP_COPY", this->code[offset]);

        case OpCode::Call:
            std::println("OP_CALL");
            break;

        case OpCode::Object:
            std::println("OP_OBJECT");
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

