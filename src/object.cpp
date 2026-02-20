#include "object.hpp"
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
    return this->value;
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
            // Create a new upvalue for this function
            UpvalueObj *upvalue = new UpvalueObj();
            this->upvalues.push_back(upvalue);
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
#endif
}

ClosureObj::~ClosureObj() {
    // Release the function
    if (this->function) {
        this->function->release();
        this->function = nullptr;
    }

    for (UpvalueObj *upvalue : this->upvalues) {
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

std::string ClosureObj::toString() const {
    return "<closure: " + (this->function ? this->function->name : "released function") + ">";
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


