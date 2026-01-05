#pragma once

#include "types.hpp"
#include <string>
#include <vector>

struct FunctionObj;
struct NativeFunctionObj;
struct NativeRegistry;

using EntryID = size_t;

struct NameEntry {
    std::string name;
    TypeID type;
    int depth;
    int line_declared = -1;
    int register_index = -1;
};

class NameTable {
    std::vector<NameEntry> entries;
    std::vector<EntryID> in_scope;
    int current_depth = 0;

  public:
    std::optional<EntryID> addName(const std::string &name, int line, TypeID type,
                                   int depth = -1);
    std::optional<EntryID> findEntryInScope(const std::string &name);

    std::vector<EntryID> getNamesInScope(int depth = 0);

    NameEntry &getEntry(EntryID id);

    void enterScope();
    void exitScope(bool pop = true);
    void clearScope();

    void putInScope(EntryID id);
    int getCurrentDepth() const;

    void printTable() const;
};

struct CompileContext {
    // The current function being compiled
    FunctionObj *function = nullptr;

    // The type registry for type management
    TypeRegistry &typeRegistry;

    // The native function registry
    NativeRegistry &nativeRegistry;

    // The name table 
    NameTable nameTable;

    // The parent compile context.
    // If this value is nullptr, this is the top-level context.
    CompileContext *next = nullptr;

    CompileContext(TypeRegistry &typeRegistry, NativeRegistry &nativeRegistry);
    CompileContext(CompileContext &parent);

    // Allocate a new register
    int allocateRegister();

    // Allocate from top
    int allocateFromTop();

    // Free a previously allocated register
    void freeRegister(int reg);

private:
    std::vector<int> free_registers;
    int max_registers = 0;
};

