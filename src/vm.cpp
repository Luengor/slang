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
#define READ_OBJECT() (frame.function->chunk.object_constants[READ_BYTE()])

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

#ifdef DEBUG_PRINT
    std::print("\n=== VM Execution Start ===\n");
#endif

    for (ever) {
#ifdef DEBUG_PRINT
        // // Print current stack size
        // std::print("Stack: {} / {}\n", this->stack.size() - frame.stack_base, this->stack.size());
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
                    // The function should have cleaned up its own stack
                    // The stack should be: [... previous stack ...][return value][function]
                    // We release the function object
                    this->stack.back().object->release();
                    this->stack.pop_back();

                    // Release the function we are returning from
                    this->call_frames.back().function->release();

                    // Pop the call frame
                    this->call_frames.pop_back();

                    break;
                }
            }

            case OpCode::Call: {
                const uint8_t arg_count = READ_BYTE();
                const Value callee = this->stack[this->stack.size() - 1 - arg_count];
                assert(callee.object);

                if (callee.object->type == Object::Type::Function) {
                    FunctionObj *function = static_cast<FunctionObj *>(callee.object);
                    function->retain(); // retain the function to release later
                    // -1 for the function itself
                    // -1 for the return value slot
                    this->call_frames.push_back(
                        CallFrame(function, this->stack.size() - arg_count - 2));
                } else {
                    NativeFunctionObj *native_function =
                        static_cast<NativeFunctionObj *>(callee.object);

                    // Call the native function
                    Value result = native_function->function_ptr(
                        arg_count > 0
                            ? &this->stack[this->stack.size() - arg_count]
                            : nullptr,
                        arg_count);

                    // Pop the arguments and the function itself
                    // The native IS responsible for releasing any objects arguments
                    this->stack.resize(this->stack.size() - arg_count - 1);

                    // Push the result onto the stack
                    this->stack.back() = result;

                    // Release the native function object
                    native_function->release();
                }
                break;
            }

            case OpCode::Constant: {
                const auto constant = READ_CONSTANT();
                this->stack.push_back(constant);
                break;
            }

            case OpCode::Object: {
                Object *object = READ_OBJECT();
                object->retain();
                this->stack.push_back({.object = object});
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

            case OpCode::I2Str: {
                FixedType val = this->stack.back().fixed;
                auto strObj = new StringObj(std::to_string(val));
                this->stack.back().object = strObj;
                break;
            }

            case OpCode::F2Str: {
                FloatingType val = this->stack.back().floating;
                auto strObj = new StringObj(std::to_string(val));
                this->stack.back().object = strObj;
                break;
            }

            case OpCode::B2Str: {
                bool val = this->stack.back().boolean;
                auto strObj = new StringObj(val ? "true" : "false");
                this->stack.back().object = strObj;
                break;
            }

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

            case OpCode::True: {
                Value val;
                val.boolean = true;
                this->stack.push_back(val);
                break;
            }

            case OpCode::False: {
                Value val;
                val.boolean = false;
                this->stack.push_back(val);
                break;
            }

            case OpCode::Pop: {
                this->stack.pop_back();
                break;
            }

            case OpCode::GetLocal: {
                const uint16_t slot = READ_UWORD();
                this->stack.push_back(frame_stack[slot]);
                break;
            }

            case OpCode::SetLocal: {
                const uint16_t slot = READ_UWORD();
                frame_stack[slot] = this->stack.back();
                break;
            }

            case OpCode::GetLocalObject: {
                const uint16_t slot = READ_UWORD();
                this->stack.push_back(frame_stack[slot]);
                // This adds a new reference to the object
                this->stack.back().object->retain();
                break;
            }

            case OpCode::SetLocalObject: {
                const uint16_t slot = READ_UWORD();
                // This removes the previous reference
                if (frame_stack[slot].object)
                    frame_stack[slot].object->release();
                // And also adds a new reference
                this->stack.back().object->retain();
                frame_stack[slot] = this->stack.back();
                break;
            }

            case OpCode::Move: {
                const uint8_t slot = READ_UWORD();
                frame_stack[slot] = this->stack.back();
                this->stack.pop_back();
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
        }

    }

    return InterpretResult::Ok;

#undef ever
#undef READ_WORD 
#undef READ_BYTE
#undef READ_INS
#undef READ_CONSTANT
}

