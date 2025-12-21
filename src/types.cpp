#include "types.hpp"

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

