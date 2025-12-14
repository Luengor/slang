#pragma once

#include "chunk.hpp"

enum class InterpretResult {
    Ok,
    CompileError,
    RuntimeError,
};

class VM {
    Chunk chunk;
    uint8_t *ip = nullptr;
    std::vector<Value> stack;

    InterpretResult run();

public:
    InterpretResult interpret(Chunk &&chunk);
};

