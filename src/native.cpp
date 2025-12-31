#include "native.hpp"
#include "object.hpp"
#include "types.hpp"
#include <cassert>
#include <iostream>

#define DUMMY_RETURN Value{.boolean = false}
#define RETURN return DUMMY_RETURN

// Native function definitions

// Only 1 string arg, but this is not enforced here
Value nativePrintF(const Value *args, size_t arg_count) {
    assert(arg_count == 1);
    StringObj *strObj = static_cast<StringObj *>(args[0].object);
    std::cout << strObj->value << std::endl;
    RETURN;
}

NativeRegistry::NativeRegistry(TypeRegistry &typeRegistry) {
    // Register all native functions
    this->native_functions["print"] = std::make_unique<NativeFunctionObj>(
        typeRegistry.getFunction(
            {typeRegistry.getPrimitive(PrimitiveKind::String)},
            typeRegistry.noneType()),
        nativePrintF);
}

NativeFunctionObj *NativeRegistry::getNativeFunction(const std::string &name) {
    auto it = this->native_functions.find(name);
    if (it != this->native_functions.end()) {
        return it->second.get();
    }
    return nullptr;
}

