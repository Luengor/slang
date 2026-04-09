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
    union {
        Value value;
        int register_index;
    } data;

    bool is_object = false;
    bool is_closed = false;

    UpvalueObj *next = nullptr; // For chaining upvalues that capture the same variable

    UpvalueObj();
    ~UpvalueObj();

    std::string toString() const override;
};

struct UpvalueInfo {
    // Whether the upvalue is an object and needs to be retained and released
    bool is_object = false;

    // Whether the upvalue refers to a local variable in the stack or another
    // upvalue in the parent closure.
    bool is_local = false;

    // The index of the local variable or the parent upvalue index. 
    int index = -1;
};

struct FunctionObj : public Object {
    // A "name" for the function
    std::string name;

    // The type ID of the function, which already encodes parameter and return
    // types
    TypeID type_id;

    // The chunk of bytecode representing the function body
    Chunk chunk;

    // The upvalues captured by this function from its parent.
    // Values of -1 means that the upvalue is created by this function
    std::vector<UpvalueInfo> upvalues;

    FunctionObj();
#ifndef NDEBUG
    ~FunctionObj();
#endif

    std::string toString() const override;
};

struct ClosureObj : public Object {
    FunctionObj *function;

    // Whether the closure is half-baked, meaning that it has its own upvalues.
    bool half_baked = false;

    std::vector<UpvalueObj *> upvalues;

#ifdef DEBUG_PRINT
    std::string function_name_cache;
#endif

    ClosureObj(FunctionObj *function, ClosureObj *parent_closure = nullptr);
    ~ClosureObj();

    void doCall();
    void doReturn();
    void create(uint8_t upvalue_index);
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

