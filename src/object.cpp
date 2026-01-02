#include "object.hpp"

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
    this->type = Object::Type::String;

#ifndef NDEBUG
    OBJECT_COUNT++;
#endif
}

std::string StringObj::toString() const {
    return this->value;
}

FunctionObj::FunctionObj() : Object() {
    this->type = Object::Type::Function;

#ifndef NDEBUG
    OBJECT_COUNT++;
#endif
}

std::string FunctionObj::toString() const {
    return this->name;
}

NativeFunctionObj::NativeFunctionObj(
    TypeID type_id, NativeFunctionPtr function_ptr, const std::string &name)
    : Object(), type_id(type_id), function_ptr(function_ptr), name(name) {
    this->type = Object::Type::NativeFunction;

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


