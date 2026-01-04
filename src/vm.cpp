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
    return this->run();
}

InterpretResult VM::run() {
#define frame this->call_frames.back()
#define registers (this->registers.data() + frame.stack_base)

#define BINARY_EXPR_2(op, from_field, to_field) { \
    const auto left_r = GET_abc_a(instruction); \
    const auto right_r = GET_abc_b(instruction); \
    const auto target_r = GET_abc_c(instruction); \
    registers[target_r].to_field = \
        registers[left_r].from_field op registers[right_r].from_field; \
    break; \
}

#define BINARY_EXPR(op, type_field) BINARY_EXPR_2(op, type_field, type_field)

#ifdef DEBUG_PRINT
    std::print("\n=== VM Execution Start ===\n");
#endif

    for (;;) {
#ifdef DEBUG_PRINT
        // // Print current stack size
        // std::print("Stack: {} / {}\n", this->stack.size() - frame.stack_base, this->stack.size());
        // Print the current instruction
        // frame.function->chunk.disassebleInstruction(static_cast<int>(this->ip - frame.function->chunk.code.data()));
#endif

        const uint32_t instruction = frame.function->chunk.code[this->ip++];
        const OpCode op = GET_op(instruction);
        switch (op) {
            case OpCode::Return: {
                if (this->call_frames.size() == 1) {
                    return InterpretResult::Ok;
                } else {
                    TODO;
                    // // The function should have cleaned up its own stack
                    // // The stack should be: [... previous stack ...][return value][function]
                    // // We release the function object
                    // this->stack.back().object->release();
                    // this->stack.pop_back();

                    // // Release the function we are returning from
                    // this->call_frames.back().function->release();

                    // // Pop the call frame
                    // this->call_frames.pop_back();

                    break;
                }
            }

            case OpCode::Constant: {
                const auto constant_index = GET_Ab_a(instruction);
                const auto target_register = GET_Ab_b(instruction);
                registers[target_register] =
                    frame.function->chunk.constants[constant_index];
                break;
            }

            case OpCode::Not: {
                const auto from_r = GET_AB_a(instruction);
                const auto to_r = GET_AB_b(instruction);
                registers[to_r].boolean = !registers[from_r].boolean;
                break;
            }

            case OpCode::NegateI: {
                const auto from_r = GET_AB_a(instruction);
                const auto to_r = GET_AB_b(instruction);
                registers[to_r].fixed = -registers[from_r].fixed;
                break;
            }

            case OpCode::NegateF: {
                const auto from_r = GET_AB_a(instruction);
                const auto to_r = GET_AB_b(instruction);
                registers[to_r].floating = -registers[from_r].floating;
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
                const auto from_r = GET_AB_a(instruction);
                const auto to_r = GET_AB_b(instruction);
                registers[to_r] = registers[from_r];
                break;
            }

            case OpCode::Jmp: {
                const auto offset = GET_sAb_a(instruction);
                this->ip += offset;
                break;
            }

            case OpCode::JmpIfFalse: {
                const auto reg = GET_sAb_b(instruction);
                if (!registers[reg].boolean) {
                    const auto offset = GET_sAb_a(instruction);
                    this->ip += offset;
                }
                break;
            }

            case OpCode::JmpIfTrue: {
                const auto reg = GET_sAb_b(instruction);
                if (registers[reg].boolean) {
                    const auto offset = GET_sAb_a(instruction);
                    this->ip += offset;
                }
                break;
            }

            case OpCode::Call: {
                TODO;
                // const uint8_t arg_count = READ_BYTE();
                // const Value callee = this->stack[this->stack.size() - 1 - arg_count];
                // assert(callee.object);

                // if (callee.object->type == Object::Type::Function) {
                //     FunctionObj *function = static_cast<FunctionObj *>(callee.object);
                //     function->retain(); // retain the function to release later
                //     // -1 for the function itself
                //     // -1 for the return value slot
                //     this->call_frames.push_back(
                //         CallFrame(function, this->stack.size() - arg_count - 2));
                // } else {
                //     NativeFunctionObj *native_function =
                //         static_cast<NativeFunctionObj *>(callee.object);

                //     // Call the native function
                //     Value result = native_function->function_ptr(
                //         arg_count > 0
                //             ? &this->stack[this->stack.size() - arg_count]
                //             : nullptr,
                //         arg_count);

                //     // Pop the arguments and the function itself
                //     // The native IS responsible for releasing any objects arguments
                //     this->stack.resize(this->stack.size() - arg_count - 1);

                //     // Push the result onto the stack
                //     this->stack.back() = result;

                //     // Release the native function object
                //     native_function->release();
                // }
                break;
            }

            case OpCode::Object: {
                TODO;
                // Object *object = READ_OBJECT();
                // object->retain();
                // this->stack.push_back({.object = object});
                break;
            }

            case OpCode::I2F: TODO; 
            case OpCode::F2I: TODO; 
            case OpCode::I2B: TODO; 
            case OpCode::B2I: TODO; 
            case OpCode::F2B: TODO; 
            case OpCode::B2F: TODO; 

            case OpCode::I2Str: {
                TODO;
                // FixedType val = this->stack.back().fixed;
                // auto strObj = new StringObj(std::to_string(val));
                // this->stack.back().object = strObj;
                break;
            }

            case OpCode::F2Str: {
                TODO;
                // FloatingType val = this->stack.back().floating;
                // auto strObj = new StringObj(std::to_string(val));
                // this->stack.back().object = strObj;
                break;
            }

            case OpCode::B2Str: {
                TODO;
                // bool val = this->stack.back().boolean;
                // auto strObj = new StringObj(val ? "true" : "false");
                // this->stack.back().object = strObj;
                break;
            }

            case OpCode::Retain: {
                TODO;
                // this->stack.back().object->retain();
                break;
            }

            case OpCode::Release: {
                TODO;
                // this->stack.back().object->release();
                // this->stack.pop_back();
                break;
            }
        }

    }

    return InterpretResult::Ok;
}

