#pragma once

#include "chunk.hpp"
#include "types.hpp"
#include <string>

struct Object {
    enum Type {
        String,
        Function,
        NativeFunction,
    };
    Type type;
    int ref_count = 1;

    virtual ~Object() = default;
    void retain();
    void release();
};

#ifndef NDEBUG
int getObjectCount();
#endif

struct StringObj : public Object {
    std::string value;

    StringObj(const std::string &value);
#ifndef NDEBUG
    ~StringObj();
#endif
};

struct FunctionObj : public Object {
    // The type ID of the function, which already encodes parameter and return
    // types
    TypeID type_id;

    // The chunk of bytecode representing the function body
    Chunk chunk;

    FunctionObj();
#ifndef NDEBUG 
    ~FunctionObj();
#endif
};


using NativeFunctionPtr = Value(*)(const Value *args, size_t arg_count);

struct NativeFunctionObj : public Object {
    // The type ID of the function
    TypeID type_id;

    // The pointer to the native function implementation
    NativeFunctionPtr function_ptr;

    // A name to find the function
    std::string name;

    NativeFunctionObj(TypeID type_id, NativeFunctionPtr function_ptr);

#ifndef NDEBUG 
    ~NativeFunctionObj();
#endif
};

