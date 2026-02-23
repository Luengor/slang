#include "object.hpp"
#include <cassert>
#include <format>
#include <stdexcept>

#ifndef NDEBUG
#ifdef DEBUG_PRINT
#include <print>
#endif

static int OBJECT_COUNT = 0;

int getObjectCount() { return OBJECT_COUNT; }

#endif



void Object::retain() {
    this->ref_count++;

#ifdef DEBUG_PRINT
    std::print("{} retained. New ref count: {}\n",
               this->toString(), this->ref_count);
#endif
}

void Object::release() {
    this->ref_count--;
    if (this->ref_count <= 0) {
        delete this;
    }
#ifdef DEBUG_PRINT
    else {
        std::print("{} released. New ref count: {}\n",
                   this->toString(), this->ref_count);
    }
#endif
}

StringObj::StringObj(const std::string &value) : Object(), value(value) {
    this->obj_type = Object::Type::String;

#ifndef NDEBUG
    OBJECT_COUNT++;
#endif
}

std::string StringObj::toString() const {
    return std::format("\"{}\"", this->value);
}

UpvalueObj::UpvalueObj() : Object() {
    this->obj_type = Object::Type::Upvalue;

#ifndef NDEBUG
    OBJECT_COUNT++;
#endif
}

UpvalueObj::~UpvalueObj() {
#ifndef NDEBUG
    OBJECT_COUNT--;
#ifdef DEBUG_PRINT
    std::print("{} destroyed. Remaining objects: {}\n",
               this->toString(), OBJECT_COUNT);
#endif
#endif
}

std::string UpvalueObj::toString() const {
    return "<upvalue>";
}

FunctionObj::FunctionObj() : Object() {
    this->obj_type = Object::Type::Function;

#ifndef NDEBUG
    OBJECT_COUNT++;
#endif
}

std::string FunctionObj::toString() const {
    return this->name;
}

ClosureObj::ClosureObj(FunctionObj *function, ClosureObj *parent_closure) : Object() {
    this->obj_type = Object::Type::Closure;

    // Get the function
    this->function = function;
    this->function->retain(); // retain the function to release it later

    // Copy the upvalues from the parent closure if it exists
    for (auto up : function->upvalues) {
        if (up == -1) {
            this->half_baked = true;

            // Placeholder for the upvalue that will be created when the closure is called
            this->upvalues.push_back(nullptr);
        } else if (parent_closure) {
            UpvalueObj *upvalue = parent_closure->upvalues[up];
            upvalue->retain();
            this->upvalues.push_back(upvalue);
        } else {
            throw std::runtime_error("Function has upvalues but no parent closure provided");
        }
    }

#ifndef NDEBUG
    OBJECT_COUNT++;

#ifdef DEBUG_PRINT
    this->function_name_cache = this->function->name;
    std::print("{} created. Total objects: {}\n",
               this->toString(), OBJECT_COUNT);
#endif
#endif
}

ClosureObj::~ClosureObj() {
    // Release the function
    if (this->function) {
        this->function->release();
        this->function = nullptr;
    }

    for (UpvalueObj *upvalue : this->upvalues) {
        if (upvalue)
            upvalue->release();
    }
    this->upvalues.clear();

#ifndef NDEBUG
    OBJECT_COUNT--;
#ifdef DEBUG_PRINT
    std::print("{} destroyed. Remaining objects: {}\n",
               this->toString(), OBJECT_COUNT);
#endif
#endif
}

void ClosureObj::doCall() {
    // If the closure is not half-baked, we have no need to do anything,
    // since all upvalues are already set up correctly.
    if (!this->half_baked)
        return;

    // If it is, create a new set of upvalues for this closure
    for (size_t i = 0; i < this->function->upvalues.size(); i++) {
        if (this->function->upvalues[i] == -1) {
            // This upvalue is created by this function, so we create a new one
            UpvalueObj *new_upvalue = new UpvalueObj();
            new_upvalue->next = this->upvalues[i]; // Chain it to the previous upvalue that captures the same variable
            this->upvalues[i] = new_upvalue; // Set the new upvalue as the current one for this variable

#ifdef DEBUG_PRINT
            std::print("created {} for {}\n",
                       new_upvalue->toString(), this->toString());
#endif
        }
    }
}

void ClosureObj::doReturn() {
    // If the closure is not half-baked, we have no need to do anything,
    // since all upvalues are already set up correctly.
    if (!this->half_baked)
        return;

    // If it is, we need to pop the upvalues created by this function
    for (size_t i = 0; i < this->function->upvalues.size(); i++) {
        if (this->function->upvalues[i] == -1) {
            // This upvalue is created by this function, so we pop it
            UpvalueObj *upvalue_to_pop = this->upvalues[i];
            assert(upvalue_to_pop != nullptr && "Upvalue to pop should not be null");

            this->upvalues[i] = upvalue_to_pop->next; // Set the next upvalue as the current one for this variable
            upvalue_to_pop->release(); // Release the popped upvalue
        }
    }
}

std::string ClosureObj::toString() const {
#ifdef DEBUG_PRINT
    return "<closure: " + this->function_name_cache + ">";
#else
    return "<closure: " + (this->function ? this->function->name : "released function") + ">";
#endif
}

NativeFunctionObj::NativeFunctionObj(
    TypeID type_id, NativeFunctionPtr function_ptr, const std::string &name)
    : Object(), type_id(type_id), function_ptr(function_ptr), name(name) {
    this->obj_type = Object::Type::NativeFunction;

#ifndef NDEBUG
    OBJECT_COUNT++;
#endif
}

std::string NativeFunctionObj::toString() const {
    return "<native: " + this->name + ">";
}

#ifndef NDEBUG


StringObj::~StringObj() {
    OBJECT_COUNT--;

#ifdef DEBUG_PRINT
    std::print("{} destroyed. Remaining objects: {}\n",
               this->toString(), OBJECT_COUNT);
#endif
}

FunctionObj::~FunctionObj() {
    OBJECT_COUNT--;

#ifdef DEBUG_PRINT
    std::print("{} destroyed. Remaining objects: {}\n",
               this->toString(), OBJECT_COUNT);
#endif
}

NativeFunctionObj::~NativeFunctionObj() {
    OBJECT_COUNT--;

#ifdef DEBUG_PRINT
    std::print("{} destroyed. Remaining objects: {}\n",
               this->toString(), OBJECT_COUNT);
#endif
}

#endif


