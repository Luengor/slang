#pragma once

#include "chunk.hpp"
#include "value.hpp"
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

using TypeID = uint32_t;

enum class PrimitiveKind {
    None,
    Fixed,
    Floating,
    String,
    Boolean,
};

struct PrimitiveType {
    PrimitiveKind kind;

    bool operator==(const PrimitiveType &other) const noexcept {
        return this->kind == other.kind;
    }
};

struct FunctionType {
    std::vector<TypeID> param_types;
    TypeID return_type;

    bool operator==(const FunctionType &other) const noexcept {
        return this->param_types == other.param_types &&
               this->return_type == other.return_type;
    }
};

using TypeData = std::variant<PrimitiveType, FunctionType>;

class TypeRegistry {
    std::vector<TypeData> types;
    std::unordered_map<std::string, TypeID> type_names;

    TypeID getOrAdd(const TypeData &typeData);

  public:
    TypeID getPrimitive(PrimitiveKind kind);
    TypeID getFunction(const std::vector<TypeID> &param_types,
                       TypeID return_type);
    TypeID getFromValue(const TypedValue &value);

    TypeID reserveTypeID(const std::string &name);
    void setTypeAlias(const std::string &name, TypeID typeID);
    void fillTypeID(TypeID typeID, const TypeData &typeData);
    std::optional<TypeID> getTypeFromName(const std::string &name);
    bool typeRecurses(TypeID typeID);

    inline TypeID noneType() { return getPrimitive(PrimitiveKind::None); }
    bool isNumeric(TypeID typeID);
    bool isPrimitive(TypeID typeID);
    bool isObject(TypeID typeID);
    bool isFunction(TypeID typeID);

    inline TypeData getTypeData(TypeID typeID) const {
        return this->types[typeID];
    }

    std::optional<OpCode> getCastOp(TypeID from, TypeID to);
    std::optional<PrimitiveKind> getCommonPrimitive(PrimitiveKind a,
                                                    PrimitiveKind b);
};
