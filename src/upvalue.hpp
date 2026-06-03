#pragma once

#include "value.hpp"

#include <memory>
#include <string>

struct UpValue;

using UpValuePtr = std::shared_ptr<UpValue>;

struct UpValueInfo {
    // Whether the upvalue is an object and needs to be retained and released
    bool is_object = false;

    // Whether the upvalue refers to a local variable in the stack or another
    // upvalue in the parent closure.
    bool is_local = false;

    // The index of the local variable or the parent upvalue index.
    int index = -1;
};

struct UpValue {
    union {
        Value value;
        int register_index;
    } data;

    bool is_object = false;
    bool is_closed = false;

    UpValuePtr next = nullptr;

    UpValue();
    ~UpValue();

    std::string toString() const;
};

