#include "compile_context.hpp"
#include "native.hpp"
#include "object.hpp"
#include <algorithm>
#include <cassert>
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

std::optional<EntryID> NameTable::findEntryInScope(const std::string &name,
                                                   bool only_upvalues) {
    // Search in reverse order to find the most recent entry
    for (auto it = this->in_scope.rbegin(); it != this->in_scope.rend(); ++it) {
        const NameEntry &entry = this->entries[*it];
        if (entry.name == name &&
            (!only_upvalues || (entry.is_upvalue || entry.is_captured))) {
            return *it;
        }
    }

    return std::nullopt; // Not found
}

std::vector<EntryID> NameTable::getNamesInScope(int depth) {
    std::vector<EntryID> result = this->in_scope;
    if (depth > 0) {
        result.erase(std::remove_if(result.begin(), result.end(),
                                    [this, depth](EntryID id) {
                                        return this->entries[id].depth < depth;
                                    }),
                     result.end());
    }

    return result;
}

const std::vector<NameEntry> &NameTable::getEntries() { return this->entries; }

NameEntry &NameTable::getEntry(EntryID id) { return this->entries[id]; }

bool NameTable::isRegisterVariableOwned(int reg) {
    for (const auto &id : this->in_scope) {
        const auto &entry = this->entries[id];
        if (entry.register_index == reg)
            return true;
    }

    return false;
}

void NameTable::enterScope() { this->current_depth++; }

void NameTable::exitScope(bool pop) {
    if (pop) {
        // Remove all entries at the given depth
        this->in_scope.erase(
            std::remove_if(this->in_scope.begin(), this->in_scope.end(),
                           [this](EntryID id) {
                               return this->entries[id].depth >=
                                      this->current_depth;
                           }),
            this->in_scope.end());
    }

    this->current_depth--;
}

void NameTable::clearScope() {
    this->in_scope.clear();
    this->current_depth = 0;
}

void NameTable::putInScope(EntryID id) { this->in_scope.push_back(id); }

int NameTable::getCurrentDepth() const { return this->current_depth; }

void NameTable::capture(EntryID id) { this->entries[id].is_captured = true; }

void NameTable::markUpvalue(EntryID id) { this->entries[id].is_upvalue = true; }

void NameTable::printTable() const {
    std::println("Name Table:");

    // Print header
    std::println(
        " Line | Name           | Type | Depth | Cap? | UpVal? | Reg ");
    std::println(
        "------|----------------|------|-------|------|--------|-----");

    for (size_t i = 0; i < this->entries.size(); i++) {
        const auto &entry = this->entries[i];
        std::println("{:>5} | {:<14} | {:<4} | {:>5} | {:>4} | {:>6} | {:>3} ",
                     entry.line_declared, entry.name, entry.type, entry.depth,
                     entry.is_captured ? "Yes" : "No",
                     entry.is_upvalue ? "Yes" : "No", entry.register_index);
    }
}

CompileContext::CompileContext(TypeRegistry &typeRegistry,
                               NativeRegistry &nativeRegistry)
    : typeRegistry(typeRegistry), nativeRegistry(nativeRegistry) {}

CompileContext::CompileContext(CompileContext &parent)
    : typeRegistry(parent.typeRegistry), nativeRegistry(parent.nativeRegistry),
      next(&parent) {}

std::optional<EntryID>
CompileContext::resolveNewUpvalue(const std::string &name) {
    // If there's no parent context, we can't resolve an upvalue
    if (this->next == nullptr)
        return std::nullopt;

    // Try to find the name in the parent context's name table
    auto parentEntry = this->next->nameTable.findEntryInScope(name);
    if (parentEntry.has_value())
        return this->markUpvalue(parentEntry.value());

    // If not found, try to resolve it as an upvalue in the parent context
    auto upvalue = this->next->resolveNewUpvalue(name);
    if (upvalue.has_value())
        return this->markUpvalue(upvalue.value());

    return std::nullopt; // Not found
}

int CompileContext::getUpvalueIndex(EntryID entry_id) {
    // Get it from the name table first
    auto &entry = this->nameTable.getEntry(entry_id);
    assert((entry.is_upvalue || entry.is_captured) &&
           "Trying to get the upvalue index of a non-upvalue entry");

    if (entry.register_index != -1) {
        // If it already has an index, return it
        return entry.register_index;
    }

    // There sould only be upvalues from this point on
    assert(entry.is_upvalue &&
           "Non-upvalue captured local should have been resolved");

    // Because local upvalues are actually normal local variables, this upvalue
    // should have an index in a parent context.
    assert(this->next != nullptr &&
           "Upvalue should have been resolved in a parent context");
    auto parent_entry_id =
        this->next->nameTable.findEntryInScope(entry.name, true);
    assert(parent_entry_id.has_value() &&
           "Upvalue should have been found in parent context");
    auto parent_entry = this->next->nameTable.getEntry(parent_entry_id.value());
    int parent_index = this->next->getUpvalueIndex(parent_entry_id.value());

    // Add it to this function's upvalues with the parent index
    auto upvalue_index = this->function->upvalues.size();
    const UpvalueInfo info{.is_object = this->typeRegistry.isObject(entry.type),

                           // If the parent entry is an upvalue, then this
                           // upvalue does not refer to a local
                           .is_local = !parent_entry.is_upvalue,

                           .index = parent_index};
    this->function->upvalues.push_back(info);

    // Write the index on the name table
    entry.register_index = upvalue_index;

    return upvalue_index;
}

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

int CompileContext::allocateFromTop() { return this->max_registers++; }

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

EntryID CompileContext::markUpvalue(EntryID parentEntryID) {
    // Mark the parent entry as captured
    this->next->nameTable.capture(parentEntryID);

    // Add a new entry for the upvalue in the current context's name table
    const auto &parentEntry = this->next->nameTable.getEntry(parentEntryID);
    auto new_entry_id = this->nameTable.addName(
        parentEntry.name, parentEntry.line_declared, parentEntry.type);
    if (!new_entry_id.has_value()) {
        throw std::runtime_error("Failed to mark upvalue: duplicate name");
    }

    // Mark the new entry as an upvalue
    this->nameTable.markUpvalue(new_entry_id.value());

    return new_entry_id.value();
}
