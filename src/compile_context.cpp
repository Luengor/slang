#include "compile_context.hpp"
#include "native.hpp"

CompileContext::CompileContext(TypeRegistry &typeRegistry,
                               NativeRegistry &nativeRegistry,
                               CompileContext *next)
    : typeRegistry(typeRegistry), nativeRegistry(nativeRegistry), next(next) {}

int CompileContext::allocateRegister() {
    // Get a register from the stack if any are free
    if (!this->free_registers.empty()) {
        int reg = this->free_registers.back();
        this->free_registers.pop_back();
        return reg;
    }

    // Otherwise, allocate a new register
    return this->max_registers++;
}

void CompileContext::freeRegister(int reg) {
    this->free_registers.push_back(reg);
}

int CompileContext::addLocal(const std::string &name, TypeID type) {
    // Check if there are any existing locals with the same name
    // defined in the same scope
    for (auto it = this->locals.rbegin(); it != this->locals.rend(); ++it) {
        if (it->name == name && it->depth == this->scope_depth) {
            return -1; // Indicate error: duplicate local
        }
    }

    this->locals.push_back(
        Local{.name = name, .type = type, .depth = this->scope_depth});
    return static_cast<int>(this->locals.size() - 1);
}

int CompileContext::findLocal(const std::string &name) {
    // Search for the local variable in reverse order (most recent first)
    for (auto it = this->locals.rbegin(); it != this->locals.rend(); ++it) {
        if (it->name == name) {
            return std::distance(it, this->locals.rend()) - 1;
        }
    }

    return -1; // Not found
}

NameResolution CompileContext::resolveName(const std::string &name) {
    // First, try to find a local variable
    int local_index = this->findLocal(name);
    if (local_index != -1) {
        return local_index;
    }

    // If not found, try to find a native function
    NativeFunctionObj *native_fn =
        this->nativeRegistry.getNativeFunction(name);
    if (native_fn != nullptr) {
        return native_fn;
    }

    // Not found
    return std::nullopt;
}

void CompileContext::enterScope() { this->scope_depth++; }

PopCount CompileContext::getPopCount() {
    PopCount pop_count;

    for (auto it = this->locals.rbegin();
         it != this->locals.rend() && it->depth > (this->scope_depth - 1); ++it) {
        // Remove local variables from this scope and count how many to pop
        if (this->typeRegistry.isObject(it->type)) {
            pop_count.objects.push_back(pop_count.total);
        }
        pop_count.total++;
    }

    return pop_count;
}

PopCount CompileContext::exitScope() {
    this->scope_depth--;

    PopCount pop_count;

    for (auto it = this->locals.rbegin();
         it != this->locals.rend() && it->depth > this->scope_depth; ++it) {
        // Remove local variables from this scope and count how many to pop
        if (this->typeRegistry.isObject(it->type)) {
            pop_count.objects.push_back(pop_count.total);
        }
        this->locals.pop_back();
        pop_count.total++;
    }

    return pop_count;
}
