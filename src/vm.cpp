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

            case OpCode::NegateF: {
                TODO;
                // this->stack.back().floating = -this->stack.back().floating;
                break;
            }

            case OpCode::NegateI: {
                TODO;
                // this->stack.back().fixed = -this->stack.back().fixed;
                break;
            }

            case OpCode::AddF: TODO;
            case OpCode::AddI: TODO;
            case OpCode::SubtractF: TODO;
            case OpCode::SubtractI: TODO;
            case OpCode::MultiplyF: TODO;
            case OpCode::MultiplyI: TODO;
            case OpCode::DivideF: TODO;
            case OpCode::DivideI: TODO;

            case OpCode::Not: {
                TODO;
                // this->stack.back().boolean = !this->stack.back().boolean;
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

            case OpCode::EqI: TODO; 
            case OpCode::NeI: TODO; 
            case OpCode::EqF: TODO; 
            case OpCode::NeF: TODO; 
            case OpCode::EqB: TODO; 
            case OpCode::NeB: TODO; 

            case OpCode::GtI: TODO; 
            case OpCode::LtI: TODO; 
            case OpCode::GeI: TODO; 
            case OpCode::LeI: TODO; 
            case OpCode::GtF: TODO; 
            case OpCode::LtF: TODO; 
            case OpCode::GeF: TODO; 
            case OpCode::LeF: TODO; 

            case OpCode::True: {
                TODO;
                // Value val;
                // val.boolean = true;
                // this->stack.push_back(val);
                break;
            }

            case OpCode::False: {
                TODO;
                // Value val;
                // val.boolean = false;
                // this->stack.push_back(val);
                break;
            }

            case OpCode::Pop: {
                TODO;
                // this->stack.pop_back();
                break;
            }

            case OpCode::GetLocal: {
                TODO;
                // const uint16_t slot = READ_UWORD();
                // this->stack.push_back(frame_stack[slot]);
                break;
            }

            case OpCode::SetLocal: {
                TODO;
                // const uint16_t slot = READ_UWORD();
                // frame_stack[slot] = this->stack.back();
                break;
            }

            case OpCode::GetLocalObject: {
                TODO;
                // const uint16_t slot = READ_UWORD();
                // this->stack.push_back(frame_stack[slot]);
                // // This adds a new reference to the object
                // this->stack.back().object->retain();
                break;
            }

            case OpCode::SetLocalObject: {
                TODO;
                // const uint16_t slot = READ_UWORD();
                // // This removes the previous reference
                // if (frame_stack[slot].object)
                //     frame_stack[slot].object->release();
                // // And also adds a new reference
                // this->stack.back().object->retain();
                // frame_stack[slot] = this->stack.back();
                break;
            }

            case OpCode::Move: {
                TODO;
                // const uint8_t slot = READ_UWORD();
                // frame_stack[slot] = this->stack.back();
                // this->stack.pop_back();
                break;
            }

            case OpCode::Jmp: {
                TODO;
                // const int16_t offset = READ_WORD();
                // frame.ip += offset;
                break;
            }

            case OpCode::JmpIfFalse: {
                TODO;
                // const int16_t offset = READ_WORD();
                // if (!this->stack.back().boolean) {
                //     frame.ip += offset;
                // }
                // // no pop!
                break;
            }

            case OpCode::JmpIfTrue: {
                TODO;
                // const int16_t offset = READ_WORD();
                // if (this->stack.back().boolean) {
                //     frame.ip += offset;
                // }
                // // no pop!
                break;
            }

            case OpCode::JmpIfFalsePop: {
                TODO;
                // const int16_t offset = READ_WORD();
                // if (!this->stack.back().boolean) {
                //     frame.ip += offset;
                // }
                // this->stack.pop_back(); // pop!
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

