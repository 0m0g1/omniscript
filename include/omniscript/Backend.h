// Backend.h
#pragma once

#include <omniscript/Statements/Statement.h>
#include <omniscript/EngineConfigs.h>

namespace Omniscript {
class Backend {
public:
    virtual ~Backend() = default;

    virtual void initialize() = 0;

    virtual void execute(
        const std::vector<std::shared_ptr<Statement>>& statements,
        const Config& config
    ) = 0;
};

} // namespace Omniscript
