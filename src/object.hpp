#pragma once

#include "chunk.hpp"
#include "types.hpp"
#include "value.hpp"
#include <string>

struct Object {
    enum Type {
        String,
        Upvalue,
        Function,
        Closure,
        NativeFunction,
    };
    Type obj_type;
    int ref_count = 1;

    virtual ~Object() = default;
    virtual std::string toString() const = 0;
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

    std::string toString() const override;
};

struct UpvalueObj : public Object {
    Value value;

    UpvalueObj();
    ~UpvalueObj();

    std::string toString() const override;
};

struct FunctionObj : public Object {
    // A "name" for the function
    std::string name;

    // The type ID of the function, which already encodes parameter and return
    // types
    TypeID type_id;

    // The chunk of bytecode representing the function body
    Chunk chunk;

    // The upvalues captured by this function from its parent
    std::vector<int> captured_upvalues;

    // The total number of upvalues this function has (including those captured
    // from its parent and those defined in its own body)
    int total_upvalues;

    FunctionObj();
#ifndef NDEBUG
    ~FunctionObj();
#endif

    std::string toString() const override;
};

struct ClosureObj : public Object {
    FunctionObj *function;

    std::vector<UpvalueObj *> upvalues;

    ClosureObj(FunctionObj *function, ClosureObj *parent_closure = nullptr);
    ~ClosureObj();

    std::string toString() const override;
};

using NativeFunctionPtr = Value(*)(const Value *args, size_t arg_count);

struct NativeFunctionObj : public Object {
    // The type ID of the function
    TypeID type_id;

    // The pointer to the native function implementation
    NativeFunctionPtr function_ptr;

    // A name to find the function
    std::string name;

    NativeFunctionObj(TypeID type_id, NativeFunctionPtr function_ptr,
                      const std::string &name);

#ifndef NDEBUG
    ~NativeFunctionObj();
#endif

    std::string toString() const override;
};

