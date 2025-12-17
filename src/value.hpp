#pragma once

#include <variant>
#include <vector>

enum class ValueType {
    // Boolean,
    // Fixed,
    Floating,
};

using FloatingType = double;
// using FixedType = int64_t;

// using Value = std::variant<bool, FixedType, double>;
union Value {
    FloatingType floating;
    // FixedType fixed;
    // bool boolean;
}; 

using TypedValue = std::pair<ValueType, Value>;

using ValueArray = std::vector<Value>;

