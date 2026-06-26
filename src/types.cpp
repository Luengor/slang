#include "types.hpp"
#include "object.hpp"
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <variant>

// overload boilerplate
template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// Cast table
using CastKey = std::pair<PrimitiveKind, PrimitiveKind>;
struct CastKeyHash {
    std::size_t operator()(const CastKey &k) const {
        return std::hash<int>()(static_cast<int>(k.first)) ^
               (std::hash<int>()(static_cast<int>(k.second)) << 1);
    }
};

#define Cast(from, to, op)                                                     \
    {{PrimitiveKind::from, PrimitiveKind::to}, OpCode::op}

std::unordered_map<CastKey, OpCode, CastKeyHash> CAST_TABLE = {
    Cast(Fixed, Floating, I2F),   Cast(Floating, Fixed, F2I),
    Cast(Fixed, Boolean, I2B),    Cast(Boolean, Fixed, B2I),
    Cast(Floating, Boolean, F2B), Cast(Boolean, Floating, B2F),
    Cast(Fixed, String, I2Str),   Cast(Floating, String, F2Str),
    Cast(Boolean, String, B2Str),
};

#undef Cast

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

TypeID TypeRegistry::getFunction(const std::vector<TypeID> &param_types,
                                 TypeID return_type) {
    return getOrAdd(FunctionType{param_types, return_type});
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
        return getPrimitive(PrimitiveKind::None);
    }

    switch (value.second.object->obj_type) {
        case Object::Type::String:
            return getPrimitive(PrimitiveKind::String);
        case Object::Type::Function:
            {
                FunctionObj *fnObj =
                    static_cast<FunctionObj *>(value.second.object);
                return fnObj->type_id;
            }
        case Object::Type::Closure:
            {
                ClosureObj *closure =
                    static_cast<ClosureObj *>(value.second.object);
                return closure->function->type_id;
            }
        case Object::Type::NativeFunction:
            {
                NativeFunctionObj *nativeFn =
                    static_cast<NativeFunctionObj *>(value.second.object);
                return nativeFn->type_id;
            }
    }

    throw std::runtime_error("Unknown object type in getFromValue.");
}

TypeID TypeRegistry::reserveTypeID(const std::string &name) {
    // Ensure it doesn't already exist
    if (this->type_names.find(name) != this->type_names.end()) {
        throw std::runtime_error("Type name already exists: " + name);
    }

    // Create a new placeholder type and return its ID
    TypeID new_id = static_cast<TypeID>(this->types.size());
    this->types.push_back(PrimitiveType{PrimitiveKind::None});
    this->type_names[name] = new_id;
    return new_id;
}

void TypeRegistry::setTypeAlias(const std::string &name, TypeID typeID) {
    // Ensure the typeID is valid
    if (typeID >= this->types.size()) {
        throw std::runtime_error("Invalid TypeID: " + std::to_string(typeID));
    }

    // Ensure the name is reserved
    auto it = this->type_names.find(name);
    if (it == this->type_names.end()) {
        throw std::runtime_error("Type name not reserved: " + name);
    }
    
    // Set the alias
    this->type_names[name] = typeID;
}

void TypeRegistry::fillTypeID(TypeID typeID, const TypeData &typeData) {
    // Ensure the typeID is valid
    if (typeID >= this->types.size()) {
        throw std::runtime_error("Invalid TypeID: " + std::to_string(typeID));
    }

    // Fill the type data
    this->types[typeID] = typeData;
}

std::optional<TypeID> TypeRegistry::getTypeFromName(const std::string &name) {
    auto it = this->type_names.find(name);
    if (it != this->type_names.end()) {
        return it->second;
    }
    return std::nullopt; // Type name not found
}

bool TypeRegistry::typeRecurses(TypeID typeID) {
    // Keep a set of the visited type IDs to detect recursion
    std::unordered_set<TypeID> visited;
    std::vector<TypeID> stack{typeID};

    while (!stack.empty()) {
        // Pop the last type ID from the stack
        TypeID current = stack.back();
        stack.pop_back();

        // If the type is a primitive, it cannot recurse
        if (this->isPrimitive(current)) {
            continue;
        }

        // If we already visited this type, we have recursion
        if (visited.find(current) != visited.end()) {
            return true;
        }

        // Mark this type as visited
        visited.insert(current);

        // If it's a function type, push its parameter and return types onto the
        // stack
        const auto &typeData = this->getTypeData(current);
        if (std::holds_alternative<FunctionType>(typeData)) {
            const FunctionType &funcType = std::get<FunctionType>(typeData);
            for (const TypeID &paramType : funcType.param_types) {
                stack.push_back(paramType);
            }
            stack.push_back(funcType.return_type);
        }
    }

    return false; // No recursion detected
}

bool TypeRegistry::isNumeric(TypeID typeID) const {
    const auto &typeData = this->getTypeData(typeID);
    if (!std::holds_alternative<PrimitiveType>(typeData)) {
        return false;
    }
    const PrimitiveType primType = std::get<PrimitiveType>(typeData);
    return primType.kind == PrimitiveKind::Fixed ||
           primType.kind == PrimitiveKind::Floating;
}

bool TypeRegistry::isPrimitive(TypeID typeID) const {
    const auto &typeData = this->getTypeData(typeID);
    return std::holds_alternative<PrimitiveType>(typeData);
}

bool TypeRegistry::isObject(TypeID typeID) const {
    const auto &typeData = this->getTypeData(typeID);
    if (std::holds_alternative<PrimitiveType>(typeData)) {
        const PrimitiveType primType = std::get<PrimitiveType>(typeData);
        return primType.kind == PrimitiveKind::String;
    }
    return true; // If not primitive, is a function
}

bool TypeRegistry::isFunction(TypeID typeID) const {
    const auto &typeData = this->getTypeData(typeID);
    return std::holds_alternative<FunctionType>(typeData);
}

std::optional<OpCode> TypeRegistry::getCastOp(TypeID from, TypeID to) {
    if (from == to)
        return std::nullopt; // No cast needed

    // Get data
    const auto &from_data = this->getTypeData(from);
    const auto &to_data = this->getTypeData(to);

    // Visit
    auto op =
        std::visit(overloaded{[&](PrimitiveType from,
                                  PrimitiveType to) -> std::optional<OpCode> {
                                  // Both are primitive types, check cast table
                                  CastKey key{from.kind, to.kind};
                                  auto it = CAST_TABLE.find(key);
                                  if (it != CAST_TABLE.end()) {
                                      return it->second;
                                  }
                                  return std::nullopt; // No valid cast found
                              },
                              [&](auto &, auto &) -> std::optional<OpCode> {
                                  // For now, any other types are not castable
                                  return std::nullopt;
                              }},
                   from_data, to_data);

    return op;
}

std::optional<PrimitiveKind> TypeRegistry::getCommonPrimitive(PrimitiveKind a,
                                                              PrimitiveKind b) {
    // If they are the same type, return that type
    if (a == b)
        return a;

    // If any of them is None, return None
    if (a == PrimitiveKind::None || b == PrimitiveKind::None) {
        return PrimitiveKind::None;
    }

    // If one of them is a boolean, promote to boolean
    if (a == PrimitiveKind::Boolean || b == PrimitiveKind::Boolean) {
        return PrimitiveKind::Boolean;
    }

    // If using numbers, promote to floating
    if ((a == PrimitiveKind::Fixed || a == PrimitiveKind::Floating) &&
        (b == PrimitiveKind::Fixed || b == PrimitiveKind::Floating)) {
        return PrimitiveKind::Floating;
    }

    return std::nullopt; // No common type found
}
