#include "vm.hpp"
#include "chunk.hpp"
#include "compiler.hpp"
#include <print>

InterpretResult VM::interpret(const std::string &source) {
    // Compile the source code into a chunk
    Compiler compiler(source);

    try {
        this->chunk = compiler.compile();
    } catch (const std::runtime_error &error) {
        std::print("Compilation error: {}\n", error.what());
        return InterpretResult::CompileError;
    }
    this->ip = this->chunk.code.data();

    // Run the chunk
    return this->run();
}

InterpretResult VM::run() {
#define ever ;;
#define READ_BYTE() (*this->ip++)
#define READ_INS() (static_cast<OpCode>(READ_BYTE()))
#define READ_CONSTANT() (this->chunk.constants[READ_BYTE()])

#define BINARY_OP(op)            \
    {                            \
        const auto b = this->stack.back(); \
        this->stack.pop_back();  \
        const auto a = this->stack.back(); \
        this->stack.pop_back();  \
        Value val {.floating = a.floating op b.floating}; \
        this->stack.push_back(val); \
        break;                   \
    }

    for (ever) {
#ifndef NDEBUG
        // Print the current instruction
        this->chunk.disassebleInstruction(static_cast<int>(this->ip - this->chunk.code.data()));
#endif

        const OpCode instruction = READ_INS();
        switch (instruction) {
            case OpCode::Return: {
                const Value value = this->stack.back();
                this->stack.pop_back();
                std::print("{:g}\n", value.floating);
                return InterpretResult::Ok;
            }

            case OpCode::Constant: {
                const auto constant = READ_CONSTANT();
                this->stack.push_back(constant);
                break;
            }

            case OpCode::Negate: {
                this->stack.back().floating = -this->stack.back().floating;
                break;
            }

            case OpCode::Add: BINARY_OP(+)
            case OpCode::Subtract: BINARY_OP(-)
            case OpCode::Multiply: BINARY_OP(*)
            case OpCode::Divide: BINARY_OP(/)
        }

    }

    return InterpretResult::Ok;

#undef ever
#undef READ_BYTE
#undef READ_INS
#undef READ_CONSTANT
}

