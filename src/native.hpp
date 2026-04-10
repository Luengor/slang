#pragma once

#include <string>
#include <unordered_map>

class TypeRegistry;
struct NativeFunctionObj;

struct NativeRegistry {
    std::unordered_map<std::string, NativeFunctionObj *> native_functions;

    NativeRegistry(TypeRegistry &typeRegistry);
    ~NativeRegistry();

    NativeFunctionObj *getNativeFunction(const std::string &name);
};
