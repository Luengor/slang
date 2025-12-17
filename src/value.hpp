#pragma once

#include <vector>

enum class ValueType {
    None,
    // Boolean,
    Fixed,
    Floating,
};

using FloatingType = double;
using FixedType = long long;

union Value {
    FloatingType floating;
    FixedType fixed;
    // bool boolean;
}; 

using TypedValue = std::pair<ValueType, Value>;

using ValueArray = std::vector<Value>;

