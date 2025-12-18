#pragma once

#include <string>
struct Object {
    enum Type {
        String,
    };
    Type type;

    virtual ~Object() = default;
};

struct StringObj : public Object {
    std::string value;
};

