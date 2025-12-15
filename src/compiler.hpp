#pragma once

#include "chunk.hpp"
#include <string>

class Compiler {
    const std::string source;

  public:
    Compiler(const std::string &source);
    Chunk compile();
};

