#pragma once

#include "types.hpp"
#include <string>
#include <vector>

struct FunctionObj;
struct NativeFunctionObj;
struct NativeRegistry;
struct AliasDeclStmt;

using EntryID = size_t;

struct NameEntry {
    std::string name;
    TypeID type;
    int depth;
    int line_declared = -1;
    int register_index = -1;

    // Is this variable captured by an inner function?
    bool is_captured = false;

    // Is this variable an upvalue (captured from an outer scope)?
    bool is_upvalue = false;
};

using AliasEntryID = size_t;

struct TypeAliasEntry {
    std::string name;
    TypeID alias_type_id;
    int depth;
    int line_declared = -1;
    AliasDeclStmt *decl = nullptr;
};

class NameTable {
    std::vector<NameEntry> entries;
    std::vector<EntryID> in_scope;
    int current_depth = 0;

  public:
    std::optional<EntryID> addName(const std::string &name, int line,
                                   TypeID type, int depth = -1);
    std::optional<EntryID> findEntryInScope(const std::string &name,
                                            bool only_upvalues = false);

    std::vector<EntryID> getNamesInScope(int depth = 0);
    const std::vector<NameEntry> &getEntries();

    NameEntry &getEntry(EntryID id);

    bool isRegisterVariableOwned(int reg);

    void enterScope();
    void exitScope(bool pop = true);
    void clearScope();

    void putInScope(EntryID id);
    int getCurrentDepth() const;

    void capture(EntryID id);
    void markUpvalue(EntryID id);

    void printTable() const;
};

class TypeAliasTable {
    std::vector<TypeAliasEntry> entries;
    std::vector<AliasEntryID> in_scope;
    int current_depth = 0;

  public:
    std::optional<AliasEntryID>
    addAlias(const std::string &name, int line, TypeID alias_type_id,
             AliasDeclStmt *decl, int depth = -1);
    std::optional<AliasEntryID> findAliasInScope(const std::string &name);
    TypeAliasEntry &getEntry(AliasEntryID id);
    void enterScope();
    void exitScope();
    int getCurrentDepth() const;
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
    TypeAliasTable typeAliasTable;

    // The parent compile context.
    // If this value is nullptr, this is the top-level context.
    CompileContext *next = nullptr;

    CompileContext(TypeRegistry &typeRegistry, NativeRegistry &nativeRegistry);
    CompileContext(CompileContext &parent);

    // Resolves the name of a new upvalue
    std::optional<EntryID> resolveNewUpvalue(const std::string &name);

    int getUpvalueIndex(EntryID entry_id);

    // Allocate a new register
    int allocateRegister();

    // Allocate from top
    int allocateFromTop();

    // Free a previously allocated register
    void freeRegister(int reg);

    // Fixup registers
    void fixupRegisters();

  private:
    std::vector<int> free_registers;
    int max_registers = 0;

    EntryID markUpvalue(EntryID parentEntryID);
};
