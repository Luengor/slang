#pragma once

#include "types.hpp"
#include <string>
#include <vector>

// Represents a local variable in the compile context
struct Local {
    // The name of the local variable
    std::string name;

    // The type of the local variable
    TypeID type;

    // The scope depth where the local variable was declared
    int depth;
};

struct PopCount {
    int total = 0;
    std::vector<int> objects = {};
};

struct FunctionObj;

struct CompileContext {
    // The current function being compiled
    FunctionObj *function = nullptr;

    // The type registry for type management
    TypeRegistry &typeRegistry;

    // The current scope depth
    int scope_depth = 0;
    // The list of local variables currently in scope
    std::vector<Local> locals = {};

    // The parent compile context.
    // If this value is nullptr, this is the top-level context.
    CompileContext *next = nullptr;

    // Adds a local variable to the current scope
    int addLocal(const std::string &name, TypeID type);

    // Finds a local variable by name and returns its index, or -1 if not found
    int findLocal(const std::string &name);

    // Enters a new scope
    void enterScope();

    // Exits the current scope and returns the number of locals to pop
    PopCount exitScope();
};

