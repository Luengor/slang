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

#define BINARY_OP(op, mode)            \
    {                            \
        const auto b = this->stack.back(); \
        this->stack.pop_back();  \
        const auto a = this->stack.back(); \
        this->stack.pop_back();  \
        Value val {.mode = a.mode op b.mode}; \
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
                std::print("{:g} {:d}\n", value.floating, value.fixed);
                return InterpretResult::Ok;
            }

            case OpCode::Constant: {
                const auto constant = READ_CONSTANT();
                this->stack.push_back(constant);
                break;
            }

            case OpCode::NegateF: {
                this->stack.back().floating = -this->stack.back().floating;
                break;
            }

            case OpCode::NegateI: {
                this->stack.back().fixed = -this->stack.back().fixed;
                break;
            }

            case OpCode::AddF: BINARY_OP(+, floating)
            case OpCode::AddI: BINARY_OP(+, fixed)
            case OpCode::SubtractF: BINARY_OP(-, floating)
            case OpCode::SubtractI: BINARY_OP(-, fixed)
            case OpCode::MultiplyF: BINARY_OP(*, floating)
            case OpCode::MultiplyI: BINARY_OP(*, fixed)
            case OpCode::DivideF: BINARY_OP(/, floating)
            case OpCode::DivideI: BINARY_OP(/, fixed)
        }

    }

    return InterpretResult::Ok;

#undef ever
#undef READ_BYTE
#undef READ_INS
#undef READ_CONSTANT
}

