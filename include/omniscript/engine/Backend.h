// Backend.h
#pragma once

#include <omniscript/engine/Statement.h>
#include <omniscript/engine/EngineConfigs.h>

class Backend {
public:
    virtual ~Backend() = default;

    virtual void initialize() = 0;

    virtual void execute(
        const std::vector<std::shared_ptr<Statement>>& statements,
        const Config& config
    ) = 0;
};
