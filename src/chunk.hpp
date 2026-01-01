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
    GetLocalLong, SetLocalLong,
};

class Chunk {
    friend class VM;
    friend struct CallFrame;

    std::vector<uint8_t> code;
    std::vector<int> lines;
    ValueArray constants = {};
    std::vector<Object *> object_constants = {};

    int simpleInstruction(const char *name, int offset);
    int simpleArgInstruction(const char *name, int offset, int arg);
    int constantInstruction(const char *name, int offset);
    int objectInstruction(const char *name, int offset);
    int jumpInstruction(const char *name, int offset);
    int disassebleInstruction(int offset);

public:
    // Write a byte to the chunk and return its offset
    unsigned write(uint8_t byte, int line = -1);

    // Write a word to the chunk and return its offset
    unsigned writeWord(uint16_t word, int line = -1);

    // Patch a word at the given offset
    void patchWord(unsigned offset, uint16_t word);

    unsigned currentOffset() const;

    template <typename T>
    inline void write(T data, int line = -1) {
        this->write(static_cast<uint8_t>(data), line);
    }

    // Adds a constant to the constants ValueArray
    int addConstant(Value value);

    // Add an object constant to the object constants array
    int addObjectConstant(Object *obj);

    // Print the chunk
    void disassemble(const std::string &header);

    // Clear the whole chunk
    void free();
};


