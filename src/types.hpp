#pragma once

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>
#include "chunk.hpp"
#include "value.hpp"

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

using TypeData = std::variant<PrimitiveType>;

class TypeRegistry {
    std::vector<TypeData> types;

    TypeID getOrAdd(const TypeData &typeData);

  public:
    TypeID getPrimitive(PrimitiveKind kind);
    TypeID getFromValue(const TypedValue &value);

    inline TypeID noneType() {
        return getPrimitive(PrimitiveKind::None);
    }
    bool isNumeric(TypeID typeID);

    inline TypeData getTypeData(TypeID typeID) const {
        return this->types[typeID];
    }

    std::optional<OpCode> getCastOp(TypeID from, TypeID to);
    std::optional<PrimitiveKind> getCommonPrimitive(PrimitiveKind a,
                                                    PrimitiveKind b);
};
