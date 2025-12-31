#include "vm.hpp"
#include "chunk.hpp"
#include "compiler.hpp"
#include "value.hpp"
#include "object.hpp"
#include <cassert>
#include <memory>
#include <print>

CallFrame::CallFrame(FunctionObj *function, size_t stack_base) {
    this->function = function;
    this->ip = function->chunk.code.data();
    this->stack_base = stack_base;
}

InterpretResult VM::interpret(const std::string &source) {
    // Compile the source code into a function 
    Compiler compiler(source);
    std::unique_ptr<FunctionObj> function;

    try {
        function = compiler.compile();
    } catch (const std::runtime_error &error) {
        std::print("Compilation error: {}\n", error.what());
        return InterpretResult::CompileError;
    }

    // Create the first call frame
    this->call_frames.push_back(
        CallFrame(function.get(), 0));

    // Run the code 
    return this->run();
}

InterpretResult VM::run() {
#define ever ;;
#define frame this->call_frames.back()
#define frame_stack (this->stack.data() + frame.stack_base)
#define READ_BYTE() (*frame.ip++)
#define READ_UWORD()                                                           \
    (frame.ip += 2, static_cast<uint16_t>((frame.ip[-2] << 8) | frame.ip[-1]))
#define READ_WORD()                                                            \
    (frame.ip += 2, static_cast<int16_t>((frame.ip[-2] << 8) | frame.ip[-1]))
#define READ_INS() (static_cast<OpCode>(READ_BYTE()))
#define READ_CONSTANT() (frame.function->chunk.constants[READ_BYTE()])

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
        frame.function->chunk.disassebleInstruction(static_cast<int>(frame.ip - frame.function->chunk.code.data()));
#endif

        const OpCode instruction = READ_INS();
        switch (instruction) {
            case OpCode::Return: {
                if (this->call_frames.size() == 1) {
                    assert(this->stack.size() == 0 && "Stack should be empty on return");
                    return InterpretResult::Ok;
                } else {
                    // Take the return value
                    const Value return_value = this->stack.back();

                    // Pop the call frame until we reach the stack base
                    const size_t stack_base = frame.stack_base;
                    this->call_frames.pop_back();
                    this->stack.resize(stack_base - 1); // to also remove the function itself

                    // Push the return value back onto the stack
                    this->stack.push_back(return_value);
                    break;
                }
            }

            case OpCode::Call: {
                const uint8_t arg_count = READ_BYTE();
                const Value callee = this->stack[this->stack.size() - 1 - arg_count];
                assert(callee.object->type == Object::Type::Function &&
                       "Callee must be a function");

                FunctionObj *function = static_cast<FunctionObj *>(callee.object);
                this->call_frames.push_back(
                    CallFrame(function, this->stack.size() - arg_count));
                break;
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

            case OpCode::EqI: BINARY_OP(==, fixed);
            case OpCode::NeI: BINARY_OP(!=, fixed);
            case OpCode::EqF: BINARY_OP(==, floating);
            case OpCode::NeF: BINARY_OP(!=, floating);
            case OpCode::EqB: BINARY_OP(==, boolean);
            case OpCode::NeB: BINARY_OP(!=, boolean);

            case OpCode::GtI: BINARY_OP(>, fixed);
            case OpCode::LtI: BINARY_OP(<, fixed);
            case OpCode::GeI: BINARY_OP(>=, fixed);
            case OpCode::LeI: BINARY_OP(<=, fixed);
            case OpCode::GtF: BINARY_OP(>, floating);
            case OpCode::LtF: BINARY_OP(<, floating);
            case OpCode::GeF: BINARY_OP(>=, floating);
            case OpCode::LeF: BINARY_OP(<=, floating);

            case OpCode::Pop: {
#ifndef NDEBUG
                const auto val = this->stack.back();
                std::println("{} {} {}", val.floating, val.fixed, val.boolean);
#endif
                this->stack.pop_back();
                break;
            }

            case OpCode::GetLocal: {
                const uint8_t slot = READ_BYTE();
                this->stack.push_back(frame_stack[slot]);
                break;
            }

            case OpCode::GetLocalLong: {
                const uint16_t slot = READ_UWORD();
                this->stack.push_back(frame_stack[slot]);
                break;
            }

            case OpCode::SetLocal: {
                const uint8_t slot = READ_BYTE();
                frame_stack[slot] = this->stack.back();
                break;
            }

            case OpCode::SetLocalLong: {
                const uint16_t slot = READ_UWORD();
                frame_stack[slot] = this->stack.back();
                break;
            }

            case OpCode::Jmp: {
                const int16_t offset = READ_WORD();
                frame.ip += offset;
                break;
            }

            case OpCode::JmpIfFalse: {
                const int16_t offset = READ_WORD();
                if (!this->stack.back().boolean) {
                    frame.ip += offset;
                }
                // no pop!
                break;
            }

            case OpCode::JmpIfTrue: {
                const int16_t offset = READ_WORD();
                if (this->stack.back().boolean) {
                    frame.ip += offset;
                }
                // no pop!
                break;
            }

            case OpCode::JmpIfFalsePop: {
                const int16_t offset = READ_WORD();
                if (!this->stack.back().boolean) {
                    frame.ip += offset;
                }
                this->stack.pop_back(); // pop!
                break;
            }

            case OpCode::Retain: {
                this->stack.back().object->retain();
                break;
            }

            case OpCode::Release: {
                this->stack.back().object->release();
                this->stack.pop_back();
                break;
            }

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
#undef READ_WORD 
#undef READ_BYTE
#undef READ_INS
#undef READ_CONSTANT
}

