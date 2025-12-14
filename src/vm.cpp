#include "vm.hpp"
#include "chunk.hpp"
#include <print>

InterpretResult VM::interpret(Chunk &&chunk) {
    // Keep the chunk
    this->chunk = std::move(chunk);
    this->ip = this->chunk.code.data();

    // Run the chunk
    return this->run();
}

InterpretResult VM::run() {
#define ever ;;
#define READ_BYTE() (*this->ip++)
#define READ_INS() (static_cast<OpCode>(READ_BYTE()))
#define READ_CONSTANT() (this->chunk.constants[READ_BYTE()])

    for (ever) {
#ifndef NDEBUG
        // Print the stack
        std::print("          ");
        for (const auto &value : this->stack) {
            std::print("[ ");
            std::print("{:g} ", value);
            std::print("] ");
        }
        std::print("\n");

        // Print the current instruction
        this->chunk.disassebleInstruction(static_cast<int>(this->ip - this->chunk.code.data()));
#endif

        const OpCode instruction = READ_INS();
        switch (instruction) {
            case OpCode::Return: {
                const Value value = this->stack.back();
                this->stack.pop_back();
                std::print("{:g}\n", value);
                return InterpretResult::Ok;
            }

            case OpCode::Constant: {
                const auto constant = READ_CONSTANT();
                this->stack.push_back(constant);
                break;
            }
        }

    }

    return InterpretResult::Ok;

#undef ever
#undef READ_BYTE
#undef READ_INS
#undef READ_CONSTANT
}

