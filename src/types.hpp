#pragma once

#include "chunk.hpp"
#include "value.hpp"
#include <cstdint>
#include <optional>
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

struct AliasType {
    std::optional<TypeID> target_type;

    bool operator==(const AliasType &other) const noexcept {
        return this->target_type == other.target_type;
    }
};

using TypeData = std::variant<PrimitiveType, FunctionType, AliasType>;

class TypeRegistry {
    std::vector<TypeData> types;

    TypeID getOrAdd(const TypeData &typeData);

  public:
    TypeID getPrimitive(PrimitiveKind kind);
    TypeID getFunction(const std::vector<TypeID> &param_types,
                       TypeID return_type);
    TypeID createAliasPlaceholder();
    void bindAlias(TypeID alias_type, TypeID target_type);
    TypeID resolveAlias(TypeID typeID) const;
    TypeID getFromValue(const TypedValue &value);

    inline TypeID noneType() { return getPrimitive(PrimitiveKind::None); }
    bool isNumeric(TypeID typeID);
    bool isPrimitive(TypeID typeID);
    bool isObject(TypeID typeID);
    bool isFunction(TypeID typeID);

    TypeData getTypeData(TypeID typeID) const;

    std::optional<OpCode> getCastOp(TypeID from, TypeID to);
    std::optional<PrimitiveKind> getCommonPrimitive(PrimitiveKind a,
                                                    PrimitiveKind b);
};
