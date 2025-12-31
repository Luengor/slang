#pragma once

#include <memory>
#include <string>
#include <unordered_map>

class TypeRegistry;
struct NativeFunctionObj;

struct NativeRegistry {
    std::unordered_map<std::string, std::unique_ptr<NativeFunctionObj>>
        native_functions;

    NativeRegistry(TypeRegistry &typeRegistry);

    NativeFunctionObj *getNativeFunction(const std::string &name);
};

