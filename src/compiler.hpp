#pragma once

#include <memory>
#include <string>

struct FunctionObj;

class Compiler {
    const std::string source;

  public:
    Compiler(const std::string &source);
    std::unique_ptr<FunctionObj> compile();
};
