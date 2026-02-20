#include "chunk.hpp"
#include "error.hpp"
#include "object.hpp"
#include <cassert>
#include <print>

Chunk::~Chunk() {
    // Release all object constants
    for (unsigned i = 1; i < this->object_constants.size(); i++) {
        this->object_constants[i]->release();
    }
}

void Chunk::disassembleJump(const char *name, int offset, bool show_reg) const {
    const auto instruction = this->code[offset];
    int32_t sBx = GET_sBx(instruction);

    int32_t jump_target = offset + 1 + sBx;

    if (show_reg) {
        uint8_t A = GET_A(instruction);
        std::println("{:<16} R{:<3d} {:04d}", name, A, jump_target);
    } else {
        std::println("{:<16} {:04d}", name, jump_target);
    }
}


void Chunk::disassembleABx(const char *name, uint32_t instruction, const std::string &b_text) const {
    uint8_t A = GET_A(instruction);
    uint32_t bx = GET_Bx(instruction);

    if (b_text.size() == 0)
        std::println("{:<16} R{:<3d}", name, A);
    else if (b_text == "RC") {
        std::println("{:<16} R{:<3d} {}{:<3d}", name, A, bx >= 256 ? 'C' : 'R', bx % 256);
    }
    else
        std::println("{:<16} R{:<3d} {}{:<4d}", name, A, b_text, bx);
}

void Chunk::disassembleAsBx(const char *name, uint32_t instruction) const {
    uint8_t A = GET_A(instruction);
    int32_t sbx = GET_sBx(instruction);

    std::println("{:<16} R{:<3d} {:<4d}", name, A, sbx);
}

void Chunk::disassembleABC(const char *name, uint32_t instruction) const {
    uint8_t a = GET_A(instruction);
    uint16_t b = GET_B(instruction);
    uint16_t c = GET_C(instruction);

    std::println("{:<16} R{:<3d} {}{:<3d} {}{:<3d}", name, a,
                 b >= 256 ? 'C' : 'R', b % 256, c >= 256 ? 'C' : 'R', c % 256);
}

void Chunk::disassembleCall(const char *name, uint32_t instruction) const {
    uint8_t a = GET_A(instruction);
    uint16_t b = GET_B(instruction);
    uint16_t c = GET_C(instruction);

    if (a == 0)
        std::println("{:<16} {}{:<3d}", name, b >= 256 ? 'O' : 'R', b % 256);
    else if (a == 1)
        std::println("{:<16} {}{:<3d} R{:d}", name, b >= 256 ? 'O' : 'R', b % 256, c);
    else
        std::println("{:<16} {}{:<3d} R{:d}...R{:d}", name, b >= 256 ? 'O' : 'R', b % 256, c, c + a - 1);
}

void Chunk::disassembleInstruction(int offset) {
    std::print("{:04d} ", offset);

    if (offset > 0 && this->lines[offset - 1] == this->lines[offset])
        std::print("   | ");
    else
        std::print("{:4d} ", this->lines[offset]);

    OpCode instruction = GET_op(this->code[offset]);
    switch (instruction) {
        case OpCode::Return: {
            uint32_t bx = GET_Bx(this->code[offset]);
            std::println("{:<16} {}{:<3d}", "OP_RETURN", bx >= 256 ? 'C' : 'R', bx % 256);
            return;
        }

        case OpCode::Constant:
            return this->disassembleABx("OP_CONSTANT", this->code[offset], "C");

        case OpCode::Object:
            return this->disassembleABx("OP_OBJECT", this->code[offset], "O");

        case OpCode::Self:
            return this->disassembleABx("OP_SELF", this->code[offset]);

        case OpCode::Closure: {
            uint8_t A = GET_A(this->code[offset]);
            uint32_t bx = GET_Bx(this->code[offset]);
            FunctionObj *func = static_cast<FunctionObj *>(this->object_constants[bx]);

            std::println("{:<16} R{:<3d} closure{}", "OP_CLOSURE", A, func->name);
            return;
        }

        case OpCode::GetUpvalue:
            return this->disassembleABx("OP_GETUPVALUE", this->code[offset], "U");

        case OpCode::SetUpvalue:
            return this->disassembleABx("OP_SETUPVALUE", this->code[offset], "U");

        case OpCode::ReleaseUpvalue:
            return this->disassembleABx("OP_RELEASEUPVALUE", this->code[offset], "U");

        case OpCode::Retain:
            return this->disassembleABx("OP_RETAIN", this->code[offset]);

        case OpCode::Release:
            return this->disassembleABx("OP_RELEASE", this->code[offset]);

        case OpCode::Not:
            return this->disassembleABx("OP_NOT", this->code[offset], "RC");

        case OpCode::NegateI:
            return this->disassembleABx("OP_NEGATE_I", this->code[offset], "RC");

        case OpCode::NegateF:
            return this->disassembleABx("OP_NEGATE_F", this->code[offset], "RC");

        case OpCode::AddF:
            return this->disassembleABC("OP_ADDF", this->code[offset]);

        case OpCode::AddI:
            return this->disassembleABC("OP_ADDI", this->code[offset]);


        case OpCode::SubtractF:
            return this->disassembleABC("OP_SUBTRACTF", this->code[offset]);

        case OpCode::SubtractI:
            return this->disassembleABC("OP_SUBTRACTI", this->code[offset]);


        case OpCode::MultiplyF:
            return this->disassembleABC("OP_MULTIPLYF", this->code[offset]);

        case OpCode::MultiplyI:
            return this->disassembleABC("OP_MULTIPLYI", this->code[offset]);


        case OpCode::DivideF:
            return this->disassembleABC("OP_DIVIDEF", this->code[offset]);

        case OpCode::DivideI:
            return this->disassembleABC("OP_DIVIDEI", this->code[offset]);


        case OpCode::EqI:
            return this->disassembleABC("OP_EQI", this->code[offset]);

        case OpCode::NeI:
            return this->disassembleABC("OP_NEI", this->code[offset]);

        case OpCode::EqF:
            return this->disassembleABC("OP_EQF", this->code[offset]);

        case OpCode::NeF:
            return this->disassembleABC("OP_NEF", this->code[offset]);

        case OpCode::EqB:
            return this->disassembleABC("OP_EQB", this->code[offset]);

        case OpCode::NeB:
            return this->disassembleABC("OP_NEB", this->code[offset]);

        case OpCode::GtI:
            return this->disassembleABC("OP_GTI", this->code[offset]);

        case OpCode::LtI:
            return this->disassembleABC("OP_LTI", this->code[offset]);

        case OpCode::GeI:
            return this->disassembleABC("OP_GEI", this->code[offset]);

        case OpCode::LeI:
            return this->disassembleABC("OP_LEI", this->code[offset]);

        case OpCode::GtF:
            return this->disassembleABC("OP_GTF", this->code[offset]);

        case OpCode::LtF:
            return this->disassembleABC("OP_LTF", this->code[offset]);

        case OpCode::GeF:
            return this->disassembleABC("OP_GEF", this->code[offset]);

        case OpCode::LeF:
            return this->disassembleABC("OP_LEF", this->code[offset]);

        case OpCode::Copy:
            return this->disassembleABx("OP_COPY", this->code[offset], "R");

        case OpCode::Jmp:
            return this->disassembleJump("OP_JMP", offset, false);

        case OpCode::JmpIfFalse:
            return this->disassembleJump("OP_JIF", offset, true);

        case OpCode::JmpIfTrue:
            return this->disassembleJump("OP_JIT", offset, true);

        case OpCode::I2F:
            return this->disassembleABx("OP_I2F", this->code[offset], "RC");

        case OpCode::F2I:
            return this->disassembleABx("OP_F2I", this->code[offset], "RC");

        case OpCode::I2B:
            return this->disassembleABx("OP_I2B", this->code[offset], "RC");

        case OpCode::B2I:
            return this->disassembleABx("OP_B2I", this->code[offset], "RC");

        case OpCode::F2B:
            return this->disassembleABx("OP_F2B", this->code[offset], "RC");

        case OpCode::B2F:
            return this->disassembleABx("OP_B2F", this->code[offset], "RC");

        case OpCode::Call:
            return this->disassembleCall("OP_CALL", this->code[offset]);
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

        default:
            std::print("Unknown opcode {}\n",
                       static_cast<uint8_t>(instruction));
            break;
    }
}

#define FIX_LINE line = line == -1 ? this->lines.empty() ? 0 : this->lines.back() : line;

// [op:6] [a:8] [b:9] [c:9]
unsigned Chunk::writeABC(OpCode op, uint8_t a, uint16_t b, uint16_t c, int line) {
    FIX_LINE;

    uint32_t ins = (static_cast<uint8_t>(op) << 26) |
                   (static_cast<uint8_t>(a) << 18) |
                   ((b & 0x1ff) << 9) |
                   (c & 0x1ff);

    this->code.push_back(ins);
    this->lines.push_back(line);

    return this->code.size() - 1;
}

unsigned Chunk::writeABx(OpCode op, uint8_t a, uint32_t Bx, int line) {
    FIX_LINE;

    uint32_t ins = (static_cast<uint8_t>(op) << 26) |
                   (static_cast<uint8_t>(a) << 18) |
                   (Bx & 0x3ffff);

    this->code.push_back(ins);
    this->lines.push_back(line);

    return this->code.size() - 1;
}

unsigned Chunk::writeAsBx(OpCode op, uint8_t a, int32_t sBx, int line) {
    FIX_LINE;

    uint32_t Bx = static_cast<uint32_t>(sBx + 0x1ffff);
    uint32_t ins = (static_cast<uint8_t>(op) << 26) |
                   (static_cast<uint8_t>(a) << 18) |
                   (Bx & 0x3ffff);

    this->code.push_back(ins);
    this->lines.push_back(line);

    return this->code.size() - 1;
}

void Chunk::patch_AsBx(unsigned offset, int32_t sBx) {
    assert(offset < this->code.size() && "Offset out of bounds in patch_AsBx");

    uint32_t Bx = static_cast<uint32_t>(sBx + 0x1ffff);
    uint32_t ins = this->code[offset];

    // Clear the existing Bx bits
    ins &= ~(0x3ffff);

    // Set the new Bx bits
    ins |= (Bx & 0x3ffff);

    this->code[offset] = ins;
}

void Chunk::pop() {
    assert(!this->code.empty() && "Cannot pop from an empty chunk");
    this->code.pop_back();
    this->lines.pop_back();
}

uint32_t Chunk::last() const {
    assert(!this->code.empty() &&
           "Cannot get last instruction of an empty chunk");
    return this->code.back();
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

