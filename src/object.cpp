#include "object.hpp"
#include <iostream>

void Object::retain() { this->ref_count++; }

void Object::release() {
    this->ref_count--;
    if (this->ref_count <= 0) {
        delete this;
    }
}

StringObj::StringObj(const std::string &value) : Object(), value(value) {
    this->type = Object::Type::String;
}

FunctionObj::FunctionObj() : Object() {
    this->type = Object::Type::Function;
}

NativeFunctionObj::NativeFunctionObj(
    TypeID type_id, NativeFunctionPtr function_ptr)
    : Object(), type_id(type_id), function_ptr(function_ptr) {
    this->type = Object::Type::NativeFunction;
}

void NativeFunctionObj::retain() {
    // do nothing
}

void NativeFunctionObj::release() {
    // do nothing
}

#ifdef DEBUG_PRINT

StringObj::~StringObj() {
    std::cout << "StringObj destroyed: " << this->value << std::endl;
}

FunctionObj::~FunctionObj() {
    std::cout << "FunctionObj destroyed" << std::endl;
}

#endif


