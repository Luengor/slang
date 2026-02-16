#pragma once

#include "object.hpp"
#include <array>

enum class InterpretResult {
    Ok,
    CompileError,
    RuntimeError,
};

struct CallFrame {
    // The closure being called
    ClosureObj *closure;

    // The instruction to return to after this function call
    uint32_t return_ip;

    // The base index in the register file for this call frame
    size_t stack_base;

    CallFrame() = default;
    CallFrame(ClosureObj *closure, uint32_t return_ip, size_t stack_base);
};

class VM {
    uint32_t ip;
    std::array<Value, 256> registers;
    std::vector<CallFrame> call_frames;

    InterpretResult run();

public:
    InterpretResult interpret(const std::string &source);
};

