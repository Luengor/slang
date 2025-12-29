#include "compile_context.hpp"

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

void CompileContext::enterScope() { this->scope_depth++; }

int CompileContext::exitScope() {
    this->scope_depth--;

    int pop_count = 0;
    for (auto it = this->locals.rbegin();
         it != this->locals.rend() && it->depth > this->scope_depth; ++it) {
        // Remove local variables from this scope and count how many to pop
        this->locals.pop_back();
        pop_count++;
    }

    return pop_count;
}
