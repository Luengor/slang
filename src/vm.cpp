#include "vm.hpp"
#include "chunk.hpp"
#include "compiler.hpp"
#include "value.hpp"
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
        this->stack.back().mode = this->stack.back().mode op b.mode; \
        break;                   \
    }

#define CAST(from, to, real_type) \
    {                             \
        this->stack.back().to = static_cast<real_type>(this->stack.back().from); \
        break;                    \
    }

#ifndef NDEBUG
    std::print("\n=== VM Execution Start ===\n");
#endif

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
                std::print("{:g} {:d} {}\n", value.floating, value.fixed, value.boolean);
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
            case OpCode::And: BINARY_OP(&&, boolean)
            case OpCode::Or: BINARY_OP(||, boolean)

            case OpCode::Not: {
                this->stack.back().boolean = !this->stack.back().boolean;
                break;
            }

            case OpCode::I2F: CAST(fixed, floating, FloatingType);
            case OpCode::F2I: CAST(floating, fixed, FixedType);
            case OpCode::I2B: CAST(fixed, boolean, bool);
            case OpCode::B2I: CAST(boolean, fixed, FixedType);
            case OpCode::F2B: CAST(floating, boolean, bool);
            case OpCode::B2F: CAST(boolean, floating, FloatingType);

            case OpCode::EqI: BINARY_OP(==, fixed)
            case OpCode::NeI: BINARY_OP(!=, fixed)
            case OpCode::EqF: BINARY_OP(==, floating)
            case OpCode::NeF: BINARY_OP(!=, floating)
            case OpCode::EqB: BINARY_OP(==, boolean)
            case OpCode::NeB: BINARY_OP(!=, boolean)

            case OpCode::GtI: BINARY_OP(>, fixed)
            case OpCode::LtI: BINARY_OP(<, fixed)
            case OpCode::GeI: BINARY_OP(>=, fixed)
            case OpCode::LeI: BINARY_OP(<=, fixed)
            case OpCode::GtF: BINARY_OP(>, floating)
            case OpCode::LtF: BINARY_OP(<, floating)
            case OpCode::GeF: BINARY_OP(>=, floating)
            case OpCode::LeF: BINARY_OP(<=, floating)

            default:
                {
                    std::print("Runtime error: Unknown opcode {}\n",
                               static_cast<int>(instruction));
                    return InterpretResult::RuntimeError;
                }
        }

    }

    return InterpretResult::Ok;

#undef ever
#undef READ_BYTE
#undef READ_INS
#undef READ_CONSTANT
}

