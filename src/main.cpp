#include "chunk.hpp"
#include "vm.hpp"
#include <fstream>
#include <iostream>
#include <print>

#ifndef NDEBUG
#include <cassert>
#include "object.hpp"
#endif

void runFile(const char *path) {
    // Read the file
    std::ifstream file(path, std::ios::in);

    if (!file.is_open()) {
        std::print("Could not open file \"{}\".\n", path);
        exit(74);
    }

    std::string source((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    // Run the file
    VM vm;
    const auto result = vm.interpret(source);

#ifndef NDEBUG
    assert(getObjectCount() == 0 &&
           "Memory leak detected: not all objects were released.");
#endif

    if (result == InterpretResult::CompileError) {
        exit(65);
    } else if (result == InterpretResult::RuntimeError) {
        exit(70);
    }
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        runFile(argv[1]);
    } else {
        std::print("Usage: {} <path>\n", argc > 0 ? argv[0] : "slang");
        return 64;
    }

    return 0;
}
