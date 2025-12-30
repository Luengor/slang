#pragma once

#include "chunk.hpp"
#include "types.hpp"
#include <string>

struct Object {
    enum Type {
        String,
        Function,
    };
    Type type;
    int ref_count = 1;

    virtual ~Object() = default;
    void retain() { ref_count++; }
    void release() {
        ref_count--;
        if (ref_count <= 0) {
            delete this;
        }
    }
};

struct StringObj : public Object {
    std::string value;
};

struct FunctionObj : public Object {
    // The type ID of the function, which already encodes parameter and return
    // types
    TypeID type_id;

    // The chunk of bytecode representing the function body
    Chunk chunk;

    FunctionObj() : Object() {
        this->type = Object::Type::Function;
    }
};

