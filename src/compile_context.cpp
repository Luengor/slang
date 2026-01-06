#include "compile_context.hpp"
#include "native.hpp"
#include <algorithm>
#include <print>

std::optional<EntryID> NameTable::addName(const std::string &name, int line,
                                          TypeID type, int depth) {
    depth = (depth == -1) ? this->current_depth : depth;

    // Check if another entry exists with the same name at the same depth
    for (const auto &id : this->in_scope) {
        if (this->entries[id].name == name &&
            this->entries[id].depth == depth) {
            return std::nullopt; // Indicate error: duplicate name
        }
    }

    // Add the new entry
    EntryID id = this->entries.size();
    this->entries.push_back(NameEntry{name, type, depth, line});
    this->in_scope.push_back(id);
    return id;
}

std::optional<EntryID> NameTable::findEntryInScope(const std::string &name) {
    // Search in reverse order to find the most recent entry
    for (auto it = this->in_scope.rbegin(); it != this->in_scope.rend(); ++it) {
        const NameEntry &entry = this->entries[*it];
        if (entry.name == name) {
            return *it;
        }
    }

    return std::nullopt; // Not found
}

std::vector<EntryID> NameTable::getNamesInScope(int depth) {
    auto result = this->in_scope;
    if (depth > 0) {
        result.erase(
            std::remove_if(
                result.begin(), result.end(),
                [this, depth](EntryID id) {
                    return this->entries[id].depth < depth;
                }),
            result.end());
    }

    return result;
}

NameEntry &NameTable::getEntry(EntryID id) {
    return this->entries[id];
}

void NameTable::enterScope() { this->current_depth++; }

void NameTable::exitScope(bool pop) {
    if (pop) {
        // Remove all entries at the given depth
        this->in_scope.erase(
            std::remove_if(
                this->in_scope.begin(), this->in_scope.end(),
                [this](EntryID id) {
                    return this->entries[id].depth >= this->current_depth;
                }),
            this->in_scope.end());
    }

    this->current_depth--;
}

void NameTable::clearScope() {
    this->in_scope.clear();
    this->current_depth = 0;
}

void NameTable::putInScope(EntryID id) {
    this->in_scope.push_back(id);
}

int NameTable::getCurrentDepth() const {
    return this->current_depth;
}

void NameTable::printTable() const {
    std::println("Name Table:");

    // Print header
    std::println(" Line | Name           | Type | Depth | Reg ");
    std::println("------|----------------|------|-------|-----");

    for (size_t i = 0; i < this->entries.size(); i++) {
        const auto &entry = this->entries[i];
        std::println("{:>5} | {:<14} | {:<4} | {:>5} | {:>3} ",
                     entry.line_declared,
                     entry.name,
                     entry.type,
                     entry.depth,
                     entry.register_index);
    }
}


CompileContext::CompileContext(TypeRegistry &typeRegistry,
                               NativeRegistry &nativeRegistry)
    : typeRegistry(typeRegistry), nativeRegistry(nativeRegistry) {}

CompileContext::CompileContext(CompileContext &parent) :
    typeRegistry(parent.typeRegistry),
    nativeRegistry(parent.nativeRegistry),
    next(&parent) {}

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

int CompileContext::allocateFromTop() {
    return this->max_registers++;
}

void CompileContext::freeRegister(int reg) {
    if (reg == this->max_registers - 1) {
        // If it's the last allocated register, just decrease the count
        this->max_registers--;
        return;
    } else {
        this->free_registers.push_back(reg);
    }
}

void CompileContext::fixupRegisters() {
    // Check if fixing is needed
    if (this->free_registers.empty())
        return;

    // Sort free registers with biggest last
    std::sort(this->free_registers.begin(), this->free_registers.end(),
              std::greater<int>());

    // Decrease max_registers if possible
    while (!this->free_registers.empty() &&
            this->free_registers.back() == this->max_registers - 1) {
        this->free_registers.pop_back();
        this->max_registers--;
    }
}

