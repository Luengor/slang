#pragma once

#include "object.hpp"

enum class InterpretResult {
    Ok,
    CompileError,
    RuntimeError,
};

struct CallFrame {
    // The function being called
    FunctionObj *function;

    // The instruction pointer within the function's chunk
    uint8_t *ip;

    // The base index in the VM's stack where this function's locals start
    size_t stack_base;

    CallFrame() = default;
    CallFrame(FunctionObj *function, size_t stack_base);
};

class VM {
    std::vector<CallFrame> call_frames;
    std::vector<Value> stack;

    InterpretResult run();

public:
    InterpretResult interpret(const std::string &source);
};

