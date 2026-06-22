#pragma once

#include "upvalue.hpp"
#include "object.hpp"
#include <vector>

enum class InterpretResult {
    Ok,
    CompileError,
    RuntimeError,
};

using RegFile_t = std::vector<Value>;

struct CallFrame {
    // The closure being called
    ClosureObj *closure;

    // The function being called
    FunctionObj *function;

    // The instruction to return to after this function call
    uint32_t return_ip;

    // The base index in the register file for this call frame
    size_t stack_base;

    // A linked list of upvalues captured on this frame
    UpValuePtr captured_upvalue = nullptr;

    CallFrame() = default;
    CallFrame(FunctionObj *function, uint32_t return_ip, size_t stack_base);
    CallFrame(ClosureObj *closure, uint32_t return_ip, size_t stack_base);

    void cleanUpvalues(RegFile_t &regs);
};

class VM {
    uint32_t ip;
    RegFile_t regs;
    std::vector<CallFrame> call_frames;

    InterpretResult run();

  public:
    InterpretResult interpret(const std::string &source);
};
