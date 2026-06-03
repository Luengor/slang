#include "upvalue.hpp"
#include "object.hpp"

#include <format>

UpValue::UpValue() : is_object(false), is_closed(false), next(nullptr) {}

UpValue::~UpValue() {
    if (this->is_object && this->is_closed) {
        Object *obj = static_cast<Object *>(this->data.value.object);
        if (obj)
            obj->release();
    }
}

std::string UpValue::toString() const {
    return std::format("<upvalue-{}>", this->is_object ? "obj" : "val");
}

