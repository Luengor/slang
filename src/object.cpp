#include "object.hpp"
#include "vm.hpp"
#include <cassert>
#include <format>
#include <vector>

#ifndef NDEBUG
#define ADD_OBJECT add_object(this)
#define REMOVE_OBJECT remove_object(this)
#else
#define ADD_OBJECT
#define REMOVE_OBJECT
#endif

#ifndef NDEBUG
#ifdef DEBUG_PRINT
#include <print>
#endif

std::vector<Object *> all_objects;

void add_object(Object *obj) {
    all_objects.push_back(obj);

#ifdef DEBUG_PRINT
    std::print("{} created. Total objects: {}\n", obj->toString(),
               getObjectCount());
#endif
}

void remove_object(Object *obj) {
    auto it = std::find(all_objects.begin(), all_objects.end(), obj);
    assert(
        it != all_objects.end() &&
        "Removing an object that is not in the object list. Double delete or "
        "memory corruption likely.");
    all_objects.erase(it);
#ifdef DEBUG_PRINT
    std::print("{} destroyed. Remaining objects: {}\n", obj->toString(),
               getObjectCount());
#endif
}

int getObjectCount() { return all_objects.size(); }

void printAllObjects() {
#ifdef DEBUG_PRINT
    std::print("All objects ({} total):\n", getObjectCount());
    for (const auto &obj : all_objects) {
        std::print("- {} {}\n", obj->toString(), obj->ref_count);
    }
#endif
}

#endif

void Object::retain() {
    this->ref_count++;

#ifdef DEBUG_PRINT
    std::print("{} retained. New ref count: {}\n", this->toString(),
               this->ref_count);
#endif
}

void Object::release() {
    this->ref_count--;
    if (this->ref_count <= 0) {
        delete this;
    }
#ifdef DEBUG_PRINT
    else {
        std::print("{} released. New ref count: {}\n", this->toString(),
                   this->ref_count);
    }
#endif
}

StringObj::StringObj(const std::string &value) : Object(), value(value) {
    this->obj_type = Object::Type::String;

    ADD_OBJECT;
}

std::string StringObj::toString() const {
    return std::format("\"{}\"", this->value);
}

FunctionObj::FunctionObj() : Object() {
    this->obj_type = Object::Type::Function;

    ADD_OBJECT;
}

std::string FunctionObj::toString() const { return this->name; }

ClosureObj::ClosureObj(FunctionObj *function, CallFrame &current_frame)
    : Object() {
    this->obj_type = Object::Type::Closure;

    // Get the function
    this->function = function;
    this->function->retain(); // retain the function to release it later
                              //
#ifndef NDEBUG
#ifdef DEBUG_PRINT
    this->function_name_cache = this->function->name;
#endif
#endif

    // Prepare the upvalues
    for (auto &upvalue_info : this->function->upvalues) {
        if (upvalue_info.is_local) {
            const int target_register =
                current_frame.stack_base + upvalue_info.index;

            // Check if there already is an upvalue capturing this local
            // variable
            auto upvalue = current_frame.captured_upvalue;
            bool found_existing = false;
            while (upvalue) {
                if (upvalue->data.register_index == target_register) {
#ifdef DEBUG_PRINT
                    std::print(
                        "{} Reusing existing upvalue capturing {} ({} + {})\n",
                        this->toString(), target_register,
                        current_frame.stack_base, upvalue_info.index);
#endif
                    this->upvalues.push_back(upvalue);
                    found_existing = true;
                    break;
                }

                upvalue = upvalue->next;
            }

            if (found_existing)
                continue;

            // If not, create a new one
            this->upvalues.push_back(std::make_shared<UpValue>());
            this->upvalues.back()->is_object = upvalue_info.is_object;
            this->upvalues.back()->data.register_index = target_register;
            this->upvalues.back()->is_closed = false;

            // Add it to the linked list of captured upvalues and retain it
            this->upvalues.back()->next = current_frame.captured_upvalue;
            current_frame.captured_upvalue = this->upvalues.back();

#ifdef DEBUG_PRINT
            std::print("{} Created new upvalue capturing {} ({} + {})\n",
                       this->toString(), target_register,
                       current_frame.stack_base, upvalue_info.index);
#endif

        } else {
            assert(current_frame.closure != nullptr &&
                   "Upvalue refers to a parent upvalue but no parent closure "
                   "provided");
            // Capture the upvalue from the parent closure
            const auto index = upvalue_info.index;
            UpValuePtr upvalue =
                current_frame.closure->upvalues[static_cast<int>(index)];
            this->upvalues.push_back(upvalue);
        }
    }

    ADD_OBJECT;
}

ClosureObj::~ClosureObj() {
    // Release the function
    if (this->function) {
        this->function->release();
        this->function = nullptr;
    }

    this->upvalues.clear();

    REMOVE_OBJECT;
}

std::string ClosureObj::toString() const {
#ifdef DEBUG_PRINT
    return "<closure: " + this->function_name_cache + ">";
#else
    return "<closure: " +
           (this->function ? this->function->name : "released function") + ">";
#endif
}

NativeFunctionObj::NativeFunctionObj(TypeID type_id,
                                     NativeFunctionPtr function_ptr,
                                     const std::string &name)
    : Object(), type_id(type_id), function_ptr(function_ptr), name(name) {
    this->obj_type = Object::Type::NativeFunction;

    ADD_OBJECT;
}

std::string NativeFunctionObj::toString() const {
    return "<native: " + this->name + ">";
}

#ifndef NDEBUG

StringObj::~StringObj() {
    REMOVE_OBJECT;
}

FunctionObj::~FunctionObj() {
    REMOVE_OBJECT;
}

NativeFunctionObj::~NativeFunctionObj() {
    REMOVE_OBJECT;
}

#endif
