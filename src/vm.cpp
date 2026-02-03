#include "vm.hpp"
#include "chunk.hpp"
#include "compiler.hpp"
#include "value.hpp"
#include "object.hpp"
#include <cassert>
#include <memory>
#include <print>

#define TODO \
    throw std::runtime_error("TODO at " + std::string(__FILE__) + ":" + std::to_string(__LINE__))

CallFrame::CallFrame(FunctionObj *function, uint32_t return_ip, size_t stack_base) {
    this->function = function;
    this->return_ip = return_ip;
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
        CallFrame(function.get(), 0, 0));

    // Run the code 
    this->ip = 0;
    const auto result = this->run();
    assert(function->ref_count == 1 &&
           "<main> ref count should be 1 after execution.");
    return result;
}

InterpretResult VM::run() {
#define frame this->call_frames.back()
#define registers (this->registers.data() + frame.stack_base)

#define RC(x) (x < 256 ? registers[x] : frame.function->chunk.constants[x - 256])

#define BINARY_EXPR_2(op, from_field, to_field) { \
    const uint8_t target_r = GET_A(instruction); \
    const uint16_t left_rc = GET_B(instruction); \
    const uint16_t right_rc = GET_C(instruction); \
    registers[target_r].to_field = \
        RC(left_rc).from_field op RC(right_rc).from_field; \
    break; \
}

#define BINARY_EXPR(op, type_field) BINARY_EXPR_2(op, type_field, type_field)

#define CAST_EXPR(from_field, to_field)                                        \
    {                                                                          \
        const uint8_t target_r = GET_A(instruction);                           \
        const uint32_t from_rc = GET_Bx(instruction);                          \
        registers[target_r].to_field =                                         \
            static_cast<decltype(registers[target_r].to_field)>(               \
                RC(from_rc).from_field);                                       \
        break;                                                                 \
    }

#ifdef DEBUG_PRINT
    std::print("\n=== VM Execution Start ===\n");
#endif

    for (;;) {
#ifdef DEBUG_PRINT
        // // Print current stack size
        // std::print("Stack: {} / {}\n", this->stack.size() - frame.stack_base, this->stack.size());
        // Print the current instruction
        frame.function->chunk.disassembleInstruction(this->ip);
#endif

        const uint32_t instruction = frame.function->chunk.code[this->ip++];
        const OpCode op = GET_op(instruction);
        switch (op) {
            case OpCode::Return: {
                if (this->call_frames.size() == 1) {
                    return InterpretResult::Ok;
                } else {
                    // Get the return value
                    const uint32_t return_rc = GET_Bx(instruction);
                    const Value return_value = RC(return_rc);

                    // Put the return value at 0 
                    registers[0] = return_value;

                    // Get the return address
                    const uint32_t return_ip = this->call_frames.back().return_ip;
                    
                    // Release the function object
                    this->call_frames.back().function->release();

                    // Pop the call frame
                    this->call_frames.pop_back();

                    // Restore the instruction pointer
                    this->ip = return_ip;

                    break;
                }
            }

            case OpCode::Call: {
                const uint16_t callee_ro = GET_B(instruction);
                Object *callee = callee_ro < 256 ?
                    registers[callee_ro].object :
                    frame.function->chunk.object_constants[callee_ro - 256];
                assert(callee);

                // Get the start of the arguments 
                const uint16_t arg_start_r = GET_C(instruction);
                
                if (callee->obj_type == Object::Type::Function) {
                    FunctionObj *function =
                        static_cast<FunctionObj *>(callee);

                    function->retain(); // retain the function to release later

                    // Put the frame starting in the callee register
                    this->call_frames.push_back(
                        CallFrame(function, this->ip,
                                  frame.stack_base + arg_start_r));

                    // Set the instruction pointer to the function's start
                    this->ip = 0;
                } else {
                    NativeFunctionObj *native_function =
                        static_cast<NativeFunctionObj *>(callee);

                    // Get the argument count
                    const uint8_t arg_count = GET_A(instruction);

                    // Call the native
                    Value result = native_function->function_ptr(
                        registers + arg_start_r,
                        arg_count);

                    // Push the result onto the first argument register 
                    registers[arg_start_r] = result;
                }
                break;
            }

            case OpCode::Constant: {
                const uint8_t reg = GET_A(instruction); 
                const uint16_t constant = GET_Bx(instruction);
                registers[reg] =
                    frame.function->chunk.constants[constant];
                break;
            }

            case OpCode::Object: {
                const uint8_t reg = GET_A(instruction); 
                const uint32_t object_index = GET_Bx(instruction);
                Object *object =
                    frame.function->chunk.object_constants[object_index];
                object->retain(); // A new reference for the register
                registers[reg].object = object;
                break;
            }

            case OpCode::Self: {
                const uint8_t reg = GET_A(instruction);
                registers[reg].object = frame.function;
                frame.function->retain(); // A new reference for the register
                break;
            }

            case OpCode::Retain: {
                const auto reg = GET_A(instruction);
                registers[reg].object->retain();
                break;
            }

            case OpCode::Release: {
                const auto reg = GET_A(instruction);
                registers[reg].object->release();
                break;
            }

            case OpCode::Not: {
                const uint8_t to_r = GET_A(instruction);
                const uint16_t from_rc = GET_Bx(instruction);
                registers[to_r].boolean = !RC(from_rc).boolean;
                break;
            }

            case OpCode::NegateI: {
                const uint8_t to_r = GET_A(instruction);
                const uint16_t from_rc = GET_Bx(instruction);
                registers[to_r].fixed = -RC(from_rc).fixed;
                break;
            }

            case OpCode::NegateF: {
                const uint8_t to_r = GET_A(instruction);
                const uint16_t from_rc = GET_Bx(instruction);
                registers[to_r].floating = -RC(from_rc).floating;
                break;
            }

            case OpCode::AddI: BINARY_EXPR(+, fixed);
            case OpCode::SubtractI: BINARY_EXPR(-, fixed);
            case OpCode::MultiplyI: BINARY_EXPR(*, fixed);
            case OpCode::DivideI: BINARY_EXPR(/, fixed);

            case OpCode::AddF: BINARY_EXPR(+, floating);
            case OpCode::SubtractF: BINARY_EXPR(-, floating);
            case OpCode::MultiplyF: BINARY_EXPR(*, floating);
            case OpCode::DivideF: BINARY_EXPR(/, floating);

            case OpCode::EqI: BINARY_EXPR_2(==, fixed, boolean);
            case OpCode::NeI: BINARY_EXPR_2(!=, fixed, boolean);
            case OpCode::GtI: BINARY_EXPR_2(>, fixed, boolean);
            case OpCode::GeI: BINARY_EXPR_2(>=, fixed, boolean);
            case OpCode::LtI: BINARY_EXPR_2(<, fixed, boolean);
            case OpCode::LeI: BINARY_EXPR_2(<=, fixed, boolean);

            case OpCode::EqF: BINARY_EXPR_2(==, floating, boolean);
            case OpCode::NeF: BINARY_EXPR_2(!=, floating, boolean);
            case OpCode::GtF: BINARY_EXPR_2(>, floating, boolean);
            case OpCode::GeF: BINARY_EXPR_2(>=, floating, boolean);
            case OpCode::LtF: BINARY_EXPR_2(<, floating, boolean);
            case OpCode::LeF: BINARY_EXPR_2(<=, floating, boolean);

            case OpCode::EqB: BINARY_EXPR_2(==, boolean, boolean);
            case OpCode::NeB: BINARY_EXPR_2(!=, boolean, boolean);

            case OpCode::Copy: {
                const uint8_t to_r = GET_A(instruction);
                const uint16_t from_r = GET_B(instruction);
                registers[to_r] = registers[from_r];
                break;
            }

            case OpCode::Jmp: {
                const int32_t offset = GET_sBx(instruction);
                this->ip += offset;
                break;
            }

            case OpCode::JmpIfFalse: {
                const uint8_t reg = GET_A(instruction);
                if (!registers[reg].boolean) {
                    const int32_t offset = GET_sBx(instruction);
                    this->ip += offset;
                }
                break;
            }

            case OpCode::JmpIfTrue: {
                const uint8_t reg = GET_A(instruction);
                if (registers[reg].boolean) {
                    const int32_t offset = GET_sBx(instruction);
                    this->ip += offset;
                }
                break;
            }

            case OpCode::I2F: CAST_EXPR(fixed, floating);
            case OpCode::F2I: CAST_EXPR(floating, fixed);
            case OpCode::I2B: CAST_EXPR(fixed, boolean);
            case OpCode::B2I: CAST_EXPR(boolean, fixed);
            case OpCode::F2B: CAST_EXPR(floating, boolean);
            case OpCode::B2F: CAST_EXPR(boolean, floating);

            case OpCode::I2Str: {
                const uint8_t to_r = GET_A(instruction);
                const uint32_t from_rc = GET_Bx(instruction);
                int64_t val = RC(from_rc).fixed;
                auto strObj = new StringObj(std::to_string(val));
                registers[to_r].object = strObj;
                break;
            }

            case OpCode::F2Str: {
                const uint8_t to_r = GET_A(instruction);
                const uint32_t from_rc = GET_Bx(instruction);
                double val = RC(from_rc).floating;
                auto strObj = new StringObj(std::to_string(val));
                registers[to_r].object = strObj;
                break;
            }

            case OpCode::B2Str: {
                const uint8_t to_r = GET_A(instruction);
                const uint32_t from_rc = GET_Bx(instruction);
                bool val = RC(from_rc).boolean;
                auto strObj = new StringObj(val ? "true" : "false");
                registers[to_r].object = strObj;
                break;
            }

        }

    }

    return InterpretResult::Ok;
}

