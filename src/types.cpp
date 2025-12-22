#include "types.hpp"
#include <stdexcept>

TypeID TypeRegistry::getOrAdd(const TypeData &typeData) {
    // Check if the type already exists
    for (size_t i = 0; i < this->types.size(); ++i) {
        if (this->types[i] == typeData) {
            return static_cast<TypeID>(i);
        }
    }

    // If not, add it and return the new TypeID
    this->types.push_back(typeData);
    return static_cast<TypeID>(this->types.size() - 1);
}

TypeID TypeRegistry::getPrimitive(PrimitiveKind kind) {
    return getOrAdd(PrimitiveType{kind});
}

TypeID TypeRegistry::getFromValue(const TypedValue &value) {
    switch (value.first) {
        case ValueType::Fixed:
            return getPrimitive(PrimitiveKind::Fixed);
        case ValueType::Floating:
            return getPrimitive(PrimitiveKind::Floating);
        case ValueType::Boolean:
            return getPrimitive(PrimitiveKind::Boolean);
        case ValueType::Object:
            break; // Handle objects below
        default:
            return getPrimitive(PrimitiveKind::None);
    }

    // Handle objects
    if (value.second.object == nullptr) {
        throw std::runtime_error("Null object in getFromValue.");
    }

    switch (value.second.object->type) {
        case Object::Type::String:
            return getPrimitive(PrimitiveKind::String);
        default:
            return getPrimitive(PrimitiveKind::None);
    }
}

bool TypeRegistry::isNumeric(TypeID typeID) {
    return typeID == this->getPrimitive(PrimitiveKind::Fixed) ||
           typeID == this->getPrimitive(PrimitiveKind::Floating);
}

