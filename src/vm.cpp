#include "vm.hpp"
#include "chunk.hpp"
#include "compiler.hpp"
#include "object.hpp"
#include "value.hpp"
#include <cassert>
#include <memory>
#include <print>

#include <tracy/Tracy.hpp>

#define TODO                                                                   \
    throw std::runtime_error("TODO at " + std::string(__FILE__) + ":" +        \
                             std::to_string(__LINE__))

CallFrame::CallFrame(ClosureObj *closure, uint32_t return_ip,
                     size_t stack_base) {
    this->closure = closure;
    this->return_ip = return_ip;
    this->stack_base = stack_base;
}

void CallFrame::cleanUpvalues(RegFile_t &regs) {
    auto upval = this->captured_upvalue;
    while (upval) {
        // Capture it
#ifdef DEBUG_PRINT
        std::println("Lifted upvalue from R{}", upval->data.register_index);
#endif
        upval->data.value = regs[upval->data.register_index];
        upval->is_closed = true;

        // No Retain needed: the compiler skipped the pre-return Release for
        // captured variables, so the existing +1 ref count transfers here.
        // Retaining would double-count ownership.

        // Release the upvalue and move to the next
        auto next_upval = upval->next;
        upval = next_upval;
    }

    // Just to be safe
    this->captured_upvalue = nullptr;
}

InterpretResult VM::interpret(const std::string &source) {
    ZoneScopedN("VM::interpret");

    // Compile the source code into a closure
    Compiler compiler(source);
    FunctionObj *function;

    try {
        function = compiler.compile().release();
    } catch (const std::runtime_error &error) {
        std::print("Compilation error: {}\n", error.what());
        return InterpretResult::CompileError;
    }

    // Wrap the function in a closure
    CallFrame dummy_frame; // We need a dummy frame to create the closure, but
                           // it won't be used
    std::unique_ptr<ClosureObj> closure =
        std::make_unique<ClosureObj>(function, dummy_frame);
    closure->function->release(); // The closure now owns the function, so
                                  // release the initial reference

    // Create the first call frame
    this->call_frames.push_back(CallFrame(closure.get(), 0, 0));

    // Run the code
    this->ip = 0;
    const auto result = this->run();
    assert(closure->ref_count == 1 &&
           "<main> ref count should be 1 after execution.");
    return result;
}

InterpretResult VM::run() {
    ZoneScopedN("VM::run");

#define frame this->call_frames.back()
#define function frame.closure->function
#define registers (this->regs.data() + frame.stack_base)

#define RC(x) (x < 256 ? registers[x] : function->chunk.constants[x - 256])

#define BINARY_EXPR_2(op, from_field, to_field)                                \
    {                                                                          \
        ZoneScopedN("VM::Op" #op #from_field #to_field);                       \
        const uint8_t target_r = GET_A(instruction);                           \
        const uint16_t left_rc = GET_B(instruction);                           \
        const uint16_t right_rc = GET_C(instruction);                          \
        registers[target_r].to_field =                                         \
            RC(left_rc).from_field op RC(right_rc).from_field;                 \
        break;                                                                 \
    }

#define BINARY_EXPR(op, type_field) BINARY_EXPR_2(op, type_field, type_field)

#define CAST_EXPR(from_field, to_field)                                        \
    {                                                                          \
        ZoneScopedN("VM::Cast" #from_field #to_field);                         \
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
        // std::print("Stack: {} / {}\n", this->stack.size() - frame.stack_base,
        // this->stack.size()); Print the current instruction
        // Print all objects
        printAllObjects();
        
        // Print the current instruction
        function->chunk.disassembleInstruction(this->ip);
#endif

        const uint32_t instruction = function->chunk.code[this->ip++];
        const OpCode op = GET_op(instruction);
        switch (op) {
            case OpCode::Return:
                {
                    ZoneScopedN("VM::Return");

                    if (this->call_frames.size() == 1) {
                        return InterpretResult::Ok;
                    } else {
                        // Clean any upvalues that need to be lifted from this
                        // frame before moving registers around
                        this->call_frames.back().cleanUpvalues(this->regs);

                        // Get the return value
                        const uint32_t return_rc = GET_Bx(instruction);
                        const Value return_value = RC(return_rc);

                        // Put the return value at 0
                        registers[0] = return_value;

                        // Get the return address
                        const uint32_t return_ip =
                            this->call_frames.back().return_ip;

                        // Return and release the running closure
                        frame.closure->release();

                        // Pop the call frame
                        this->call_frames.pop_back();

                        // Restore the instruction pointer
                        this->ip = return_ip;

                        break;
                    }
                }

            case OpCode::Call:
                {
                    ZoneScopedN("VM::Call");

                    const uint16_t callee_ro = GET_B(instruction);
                    Object *callee =
                        callee_ro < 256
                            ? registers[callee_ro].object
                            : function->chunk.object_constants[callee_ro - 256];

                    // Handle self calls
                    callee = callee == function ? frame.closure : callee;
                    assert(callee);

                    // Get the start of the arguments
                    const uint16_t arg_start_r = GET_C(instruction);

                    if (callee->obj_type == Object::Type::NativeFunction) {
                        NativeFunctionObj *native_function =
                            static_cast<NativeFunctionObj *>(callee);

                        // Get the argument count
                        const uint8_t arg_count = GET_A(instruction);

                        // Call the native
                        Value result = native_function->function_ptr(
                            registers + arg_start_r, arg_count);

                        // Push the result onto the first argument register
                        registers[arg_start_r] = result;
                    } else {
                        ClosureObj *closure;

                        if (callee->obj_type == Object::Type::Closure) {
                            closure = static_cast<ClosureObj *>(callee);
                            closure->retain();
                        } else if (callee->obj_type == Object::Type::Function) {
                            assert(static_cast<FunctionObj *>(callee)
                                           ->upvalues.size() == 0 &&
                                   "Function objects with upvalues should be "
                                   "wrapped in closures.");
                            closure = new ClosureObj(
                                static_cast<FunctionObj *>(callee), frame);
                        } else
                            assert(false &&
                                   "Callee must be a function or closure");

                        // The closure is retained if it already existed, or
                        // newly created, so we don't need to retain it here

                        // Put the frame starting in the callee register
                        this->call_frames.push_back(CallFrame(
                            closure, this->ip, frame.stack_base + arg_start_r));

                        // Set the instruction pointer to the function's start
                        this->ip = 0;
                    }
                    break;
                }

            case OpCode::Constant:
                {
                    ZoneScopedN("VM::Constant");

                    const uint8_t reg = GET_A(instruction);
                    const uint32_t constant = GET_Bx(instruction);
                    registers[reg] = function->chunk.constants[constant];
                    break;
                }

            case OpCode::Object:
                {
                    ZoneScopedN("VM::Object");

                    const uint8_t reg = GET_A(instruction);
                    const uint32_t object_index = GET_Bx(instruction);
                    Object *object =
                        function->chunk.object_constants[object_index];
                    object->retain(); // A new reference for the register
                    registers[reg].object = object;
                    break;
                }

            case OpCode::Self:
                {
                    const uint8_t reg = GET_A(instruction);
                    registers[reg].object = frame.closure;
                    function->retain(); // A new reference for the register
                    break;
                }

            case OpCode::Closure:
                {
                    ZoneScopedN("VM::Closure");

                    const uint8_t reg = GET_A(instruction);
                    const uint32_t function_o = GET_Bx(instruction);
                    FunctionObj *func = static_cast<FunctionObj *>(
                        function->chunk.object_constants[function_o]);
                    ClosureObj *closure = new ClosureObj(func, frame);
                    registers[reg].object = closure;
                    break;
                }

            case OpCode::GetUpvalue:
                {
                    ZoneScopedN("VM::GetUpvalue");

                    const uint8_t to_r = GET_A(instruction);
                    const uint32_t upvalue_index = GET_Bx(instruction);

                    // Get the upvalue from the closure
                    const auto &upval = frame.closure->upvalues[upvalue_index];

                    // Get its value and put it in the register
                    registers[to_r] =
                        upval->is_closed
                            ? upval->data.value
                            : this->regs[upval->data.register_index];

                    // If it was an object, retain it for the register
                    if (upval->is_object) {
                        registers[to_r].object->retain();
                    }

                    break;
                }

            case OpCode::SetUpvalue:
                {
                    ZoneScopedN("VM::SetUpvalue");

                    const uint8_t upvalue_index = GET_A(instruction);
                    const uint32_t from_rc = GET_Bx(instruction);

                    // Get the upvalue from the closure
                    const auto &upval = frame.closure->upvalues[upvalue_index];

                    // If we are going to overwrite an object upvalue, release
                    // it first
                    if (upval->is_object) {
                        (upval->is_closed
                             ? static_cast<Object *>(upval->data.value.object)
                             : this->regs[upval->data.register_index].object)
                            ->release();
                    }

                    // Set the upvalue to the value from the register or
                    // constant
                    const auto &new_value = RC(from_rc);
                    (upval->is_closed
                         ? upval->data.value
                         : this->regs[upval->data.register_index]) = new_value;

                    // If it is an object, retain it for the upvalue
                    if (upval->is_object)
                        new_value.object->retain();

                    break;
                }

            case OpCode::LiftUpvalue:
                {
                    ZoneScopedN("VM::LiftUpvalue");

                    const int absolute_register =
                        frame.stack_base + GET_Bx(instruction);
                    const auto &target_value = this->regs[absolute_register];

#ifdef DEBUG_PRINT
                    std::print("Lifting upvalue capturing R{} ({} + {})\n",
                               absolute_register, frame.stack_base,
                               GET_Bx(instruction));
#endif

                    // Iterate through the linked list to find the upvalues that
                    // capture the given register
                    UpValuePtr prev = nullptr;
                    auto upval = frame.captured_upvalue;
                    while (upval) {
                        if (upval->data.register_index == absolute_register) {
                            break;
                        }

                        prev = upval;
                        upval = upval->next;
                    }

                    // The upvalue hasn't been closed over
                    // There is nothing to do
                    if (!upval)
                        break;

                    // If its reference count is only 1 (the reference on this
                    // list) don't do anything
                    if (upval.use_count() != 1) {
                        // Close it
                        assert(!upval->is_closed &&
                               "Upvalue should not already be closed");
                        upval->data.value = target_value;
                        upval->is_closed = true;

                        // Retain the value for the upvalue
                        if (upval->is_object) {
                            target_value.object->retain();
                        }
                    }

                    // Remove it from the linked list of captured upvalues and
                    // release it
                    (prev ? prev->next : frame.captured_upvalue) = upval->next;

                    break;
                }

            case OpCode::Retain:
                {
                    ZoneScopedN("VM::Retain");

                    const auto reg = GET_A(instruction);
                    registers[reg].object->retain();
                    break;
                }

            case OpCode::Release:
                {
                    ZoneScopedN("VM::Release");

                    const auto reg = GET_A(instruction);
                    registers[reg].object->release();
                    break;
                }

            case OpCode::Not:
                {
                    const uint8_t to_r = GET_A(instruction);
                    const uint32_t from_rc = GET_Bx(instruction);
                    registers[to_r].boolean = !RC(from_rc).boolean;
                    break;
                }

            case OpCode::NegateI:
                {
                    const uint8_t to_r = GET_A(instruction);
                    const uint32_t from_rc = GET_Bx(instruction);
                    registers[to_r].fixed = -RC(from_rc).fixed;
                    break;
                }

            case OpCode::NegateF:
                {
                    const uint8_t to_r = GET_A(instruction);
                    const uint32_t from_rc = GET_Bx(instruction);
                    registers[to_r].floating = -RC(from_rc).floating;
                    break;
                }

            case OpCode::AddI:
                BINARY_EXPR(+, fixed);
            case OpCode::SubtractI:
                BINARY_EXPR(-, fixed);
            case OpCode::MultiplyI:
                BINARY_EXPR(*, fixed);
            case OpCode::DivideI:
                BINARY_EXPR(/, fixed);

            case OpCode::AddF:
                BINARY_EXPR(+, floating);
            case OpCode::SubtractF:
                BINARY_EXPR(-, floating);
            case OpCode::MultiplyF:
                BINARY_EXPR(*, floating);
            case OpCode::DivideF:
                BINARY_EXPR(/, floating);

            case OpCode::EqI:
                BINARY_EXPR_2(==, fixed, boolean);
            case OpCode::NeI:
                BINARY_EXPR_2(!=, fixed, boolean);
            case OpCode::GtI:
                BINARY_EXPR_2(>, fixed, boolean);
            case OpCode::GeI:
                BINARY_EXPR_2(>=, fixed, boolean);
            case OpCode::LtI:
                BINARY_EXPR_2(<, fixed, boolean);
            case OpCode::LeI:
                BINARY_EXPR_2(<=, fixed, boolean);

            case OpCode::EqF:
                BINARY_EXPR_2(==, floating, boolean);
            case OpCode::NeF:
                BINARY_EXPR_2(!=, floating, boolean);
            case OpCode::GtF:
                BINARY_EXPR_2(>, floating, boolean);
            case OpCode::GeF:
                BINARY_EXPR_2(>=, floating, boolean);
            case OpCode::LtF:
                BINARY_EXPR_2(<, floating, boolean);
            case OpCode::LeF:
                BINARY_EXPR_2(<=, floating, boolean);

            case OpCode::EqB:
                BINARY_EXPR_2(==, boolean, boolean);
            case OpCode::NeB:
                BINARY_EXPR_2(!=, boolean, boolean);

            case OpCode::Copy:
                {
                    ZoneScopedN("VM::Copy");

                    const uint8_t to_r = GET_A(instruction);
                    const uint32_t from_r = GET_Bx(instruction);
                    registers[to_r] = registers[from_r];
                    break;
                }

            case OpCode::Jmp:
                {
                    ZoneScopedN("VM::Jmp");

                    const int32_t offset = GET_sBx(instruction);
                    this->ip += offset;
                    break;
                }

            case OpCode::JmpIfFalse:
                {
                    ZoneScopedN("VM::JmpIfFalse");

                    const uint8_t reg = GET_A(instruction);
                    if (!registers[reg].boolean) {
                        const int32_t offset = GET_sBx(instruction);
                        this->ip += offset;
                    }
                    break;
                }

            case OpCode::JmpIfTrue:
                {
                    ZoneScopedN("VM::JmpIfTrue");

                    const uint8_t reg = GET_A(instruction);
                    if (registers[reg].boolean) {
                        const int32_t offset = GET_sBx(instruction);
                        this->ip += offset;
                    }
                    break;
                }

            case OpCode::I2F:
                CAST_EXPR(fixed, floating);
            case OpCode::F2I:
                CAST_EXPR(floating, fixed);
            case OpCode::I2B:
                CAST_EXPR(fixed, boolean);
            case OpCode::B2I:
                CAST_EXPR(boolean, fixed);
            case OpCode::F2B:
                CAST_EXPR(floating, boolean);
            case OpCode::B2F:
                CAST_EXPR(boolean, floating);

            case OpCode::I2Str:
                {
                    const uint8_t to_r = GET_A(instruction);
                    const uint32_t from_rc = GET_Bx(instruction);
                    int64_t val = RC(from_rc).fixed;
                    auto strObj = new StringObj(std::to_string(val));
                    registers[to_r].object = strObj;
                    break;
                }

            case OpCode::F2Str:
                {
                    const uint8_t to_r = GET_A(instruction);
                    const uint32_t from_rc = GET_Bx(instruction);
                    double val = RC(from_rc).floating;
                    auto strObj = new StringObj(std::to_string(val));
                    registers[to_r].object = strObj;
                    break;
                }

            case OpCode::B2Str:
                {
                    const uint8_t to_r = GET_A(instruction);
                    const uint32_t from_rc = GET_Bx(instruction);
                    bool val = RC(from_rc).boolean;
                    auto strObj = new StringObj(val ? "true" : "false");
                    registers[to_r].object = strObj;
                    break;
                }
        }

        FrameMark;
    }

    return InterpretResult::Ok;
}
