#pragma once

#include "value.hpp"
#include <cstdint>
#include <string>
#include <vector>

enum class OpCode : uint8_t {
    Return, Call,
    Constant, Object,
    NegateF, NegateI,
    AddF, AddI,
    SubtractF, SubtractI,
    MultiplyF, MultiplyI,
    DivideF, DivideI,
    Not,

    EqI, NeI,
    EqF, NeF,
    EqB, NeB,

    GtI, LtI, GeI, LeI,
    GtF, LtF, GeF, LeF,

    I2F, F2I,
    I2B, B2I,
    F2B, B2F,
    I2Str, F2Str, B2Str,

    Jmp, JmpIfFalse, JmpIfFalsePop,
    JmpIfTrue,

    True, False,
    Pop, Retain, Release,
    GetLocal, SetLocal, Move,
    GetLocalObject, SetLocalObject,
};

class Chunk {
    friend class VM;
    friend struct CallFrame;

    std::vector<uint32_t> code;
    std::vector<int> lines;
    ValueArray constants = {};
    std::vector<Object *> object_constants = {};

    void disassebleInstruction(int offset);

public:

    Chunk() = default;
    ~Chunk();
    Chunk(const Chunk &) = delete;
    Chunk &operator=(const Chunk &) = delete;
    Chunk(Chunk &&) = default;
    Chunk &operator=(Chunk &&) = default;

    unsigned write_abc(OpCode op, uint8_t a, uint8_t b, uint8_t c, int line = -1);
    unsigned write_sA(OpCode op, int16_t A, int line = -1);
    unsigned write_A(OpCode op, uint16_t A, int line = -1);
    unsigned write_AB(OpCode op, uint16_t A, uint16_t B,
                      int line = -1);
    unsigned write_Ab(OpCode op, uint16_t A, uint8_t b,
                      int line = -1);

    void patch_sA(unsigned offset, int16_t A);

    unsigned currentOffset() const;

    // Adds a constant to the constants ValueArray
    int addConstant(Value value);

    // Add an object constant to the object constants array
    int addObjectConstant(Object *obj);

    // Print the chunk
    void disassemble(const std::string &header);
};

// any -> [OpCode:8] [..]
#define GET_op(ins) ((OpCode)(ins >> 24))
// abc -> [OpCode:8] [A:8] [B:8] [C:8]
#define GET_abc_a(ins) ((ins >> 16) & 0xFF)
#define GET_abc_b(ins) ((ins >> 8) & 0xFF)
#define GET_abc_c(ins) (ins & 0xFF)
// Sa -> [OpCode:8] [Unused:8] [A:16]
#define GET_sA_a(ins) ((int16_t)(ins & 0xFFFF))
// A -> [OpCode:8] [Unused:8] [A:16]
#define GET_A_a(ins) ((uint16_t)(ins & 0xFFFF))
// AB -> [OpCode:8] [A:12] [B:12]
#define GET_AB_a(ins) ((uint16_t)((ins >> 12) & 0x0FFF))
#define GET_AB_b(ins) ((uint16_t)(ins & 0x0FFF))
// Ab -> [OpCode:8] [A:16] [b:8]
#define GET_Ab_a(ins) ((uint16_t)((ins >> 8) & 0xFFFF))
#define GET_Ab_b(ins) ((uint8_t)(ins & 0xFF))

