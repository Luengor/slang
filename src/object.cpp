#include "object.hpp"

#ifndef NDEBUG
#ifdef DEBUG_PRINT
#include <print>
#endif

static int OBJECT_COUNT = 0;

int getObjectCount() { return OBJECT_COUNT; }

#endif



void Object::retain() { this->ref_count++; }

void Object::release() {
    this->ref_count--;
    if (this->ref_count <= 0) {
        delete this;
    }
}

StringObj::StringObj(const std::string &value) : Object(), value(value) {
    this->type = Object::Type::String;

#ifndef NDEBUG
    OBJECT_COUNT++;
#endif
}

FunctionObj::FunctionObj() : Object() {
    this->type = Object::Type::Function;

#ifndef NDEBUG
    OBJECT_COUNT++;
#endif
}

NativeFunctionObj::NativeFunctionObj(
    TypeID type_id, NativeFunctionPtr function_ptr)
    : Object(), type_id(type_id), function_ptr(function_ptr) {
    this->type = Object::Type::NativeFunction;

#ifndef NDEBUG
    OBJECT_COUNT++;
#endif
}

#ifndef NDEBUG 


StringObj::~StringObj() {
    OBJECT_COUNT--;

#ifdef DEBUG_PRINT
    std::print("StringObj ({}) destroyed. Remaining objects: {}\n",
               this->value, OBJECT_COUNT);
#endif
}

FunctionObj::~FunctionObj() {
    OBJECT_COUNT--;

#ifdef DEBUG_PRINT
    std::print("FunctionObj@{} destroyed. Remaining objects: {}\n",
               static_cast<const void *>(this), OBJECT_COUNT);
#endif
}

NativeFunctionObj::~NativeFunctionObj() {
    OBJECT_COUNT--;

#ifdef DEBUG_PRINT
    std::print("NativeFunctionObj ({}) destroyed. Remaining objects: {}\n",
               this->name, OBJECT_COUNT);
#endif
}

#endif


