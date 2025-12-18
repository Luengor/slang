#pragma once

#include "object.hpp"
#include <vector>

// Values

enum class ValueType {
    None,
    // Boolean,
    Fixed,
    Floating,
    Object,
};

using FloatingType = double;
using FixedType = long long;
using PtrType = Object*;

union Value {
    FloatingType floating;
    FixedType fixed;
    PtrType object;
    // bool boolean;
}; 

using TypedValue = std::pair<ValueType, Value>;

using ValueArray = std::vector<Value>;


