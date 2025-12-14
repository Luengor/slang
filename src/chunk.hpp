#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class OpCode : uint8_t {
    Return,
};

class Chunk {
    std::vector<uint8_t> code;

    int disassebleInstruction(int offset);

public:
    // Write a byte to the chunk
    void write(uint8_t byte);

    template <typename T>
    void write(T data) {
        this->write(static_cast<uint8_t>(data));
    }

    // Print the chunk
    void disassemble(const std::string &header);

    // Clear the whole chunk
    void free();
};


