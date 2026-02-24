// include/omniscript/backends/Backend.h
#pragma once
#include <omniscript/EngineConfigs.h>
#include <omniscript/ast/Ast.h>

namespace Omniscript {

class Backend {
public:
    virtual ~Backend() = default;
    virtual void initialize(const Config& config) = 0;

    // Execute = codegen + run (JIT), or codegen + emit (AOT)
    virtual void execute(const Program& program, const Config& config) = 0;
};

} // namespace Omniscript