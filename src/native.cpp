#include "native.hpp"
#include "object.hpp"
#include "types.hpp"
#include "value.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

#include <tracy/Tracy.hpp>

#define DUMMY_RETURN                                                           \
    Value { .boolean = false }
#define RETURN return DUMMY_RETURN

// Native function definitions

// Only 1 string arg, but this is not enforced here
Value nativePrintF(const Value *args, size_t arg_count) {
    ZoneScopedN("nativePrintF");

    assert(arg_count == 1);
    StringObj *strObj = static_cast<StringObj *>(args[0].object);
    std::cout << strObj->value << std::endl;

    // Decrease ref count of the string object
    strObj->release();

    RETURN;
}

Value nativeSqrtF(const Value *args, size_t arg_count) {
    ZoneScopedN("nativeSqrtF");

    assert(arg_count == 1);
    FloatingType input = args[0].floating;
    FloatingType result = std::sqrt(input);
    return Value{.floating = result};
}

NativeRegistry::NativeRegistry(TypeRegistry &typeRegistry) {
    // Register all native functions
    this->native_functions["print"] = new NativeFunctionObj(
        typeRegistry.getFunction(
            {typeRegistry.getPrimitive(PrimitiveKind::String)},
            typeRegistry.noneType()),
        nativePrintF, "print");

    this->native_functions["sqrt"] = new NativeFunctionObj(
        typeRegistry.getFunction(
            {typeRegistry.getPrimitive(PrimitiveKind::Floating)},
            typeRegistry.getPrimitive(PrimitiveKind::Floating)),
        nativeSqrtF, "sqrt");
}

NativeRegistry::~NativeRegistry() {
    // Release all native function objects
    for (auto &pair : this->native_functions) {
        pair.second->release();
    }
}

NativeFunctionObj *NativeRegistry::getNativeFunction(const std::string &name) {
    auto it = this->native_functions.find(name);
    if (it != this->native_functions.end()) {
        return it->second;
    }
    return nullptr;
}
