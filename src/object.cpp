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

#ifndef NDEBUG

StringObj::~StringObj() {
    std::cout << "StringObj destroyed: " << this->value << std::endl;
}

FunctionObj::~FunctionObj() {
    std::cout << "FunctionObj destroyed" << std::endl;
}
#endif


