#pragma once

#include "value.hpp"
#include <cstdint>
#include <string>
#include <vector>

enum class OpCode : uint8_t {
    Return, Constant, Object, Self,
    Not,
    NegateI, NegateF,

    AddI, SubtractI, MultiplyI, DivideI,
    AddF, SubtractF, MultiplyF, DivideF, 

    EqI, NeI, GtI, LtI, GeI, LeI,
    EqF, NeF, GtF, LtF, GeF, LeF,
    EqB, NeB,

    Copy,
    Retain, Release,

    Jmp, JmpIfFalse, JmpIfTrue,

    I2F, F2I,
    I2B, B2I,
    F2B, B2F,

    Call, CallSelf,

    I2Str, F2Str, B2Str,
};

class Chunk {
    friend class VM;
    friend struct CallFrame;

    std::vector<uint32_t> code;
    std::vector<int> lines;
    ValueArray constants = {};
    std::vector<Object *> object_constants = {};

    void disassembleJump(const char *name, int offset, bool show_reg) const;
    void disassembleABx(const char *name, uint32_t instruction, const std::string &b_text = "") const;
    void disassembleAsBx(const char *name, uint32_t instruction) const;
    void disassembleABC(const char *name, uint32_t instruction) const;
    void disassembleCall(const char *name, uint32_t instruction) const;
    void disassembleInstruction(int offset);

public:

    Chunk() = default;
    ~Chunk();
    Chunk(const Chunk &) = delete;
    Chunk &operator=(const Chunk &) = delete;
    Chunk(Chunk &&) = default;
    Chunk &operator=(Chunk &&) = default;

    unsigned writeABC(OpCode op, uint8_t a, uint16_t b, uint16_t c, int line = -1);
    unsigned writeABx(OpCode op, uint8_t a, uint32_t Bx, int line = -1);
    unsigned writeAsBx(OpCode op, uint8_t a, int32_t sBx, int line = -1);

    void patch_AsBx(unsigned offset, int32_t sBx);

    unsigned currentOffset() const;

    // Adds a constant to the constants ValueArray
    int addConstant(Value value);

    // Add an object constant to the object constants array
    int addObjectConstant(Object *obj);

    // Print the chunk
    void disassemble(const std::string &header);
};

// any -> [OpCode:6] [A:8] [...:18]
#define GET_op(ins) ((OpCode)(ins >> 26))
#define GET_A(ins) ((ins >> 18) & 0xFF)


// ABC -> [OpCode:6] [A:8] [B:9] [C:9] 
#define GET_B(ins) ((ins >> 9) & 0x1FF)
#define GET_C(ins) (ins & 0x1FF)
#define GET_Bx(ins) (ins & 0x3FFFF)
#define GET_sBx(ins) ((int32_t)((ins & 0x3FFFF) - 0x1FFFF))

