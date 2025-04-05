#pragma once

#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/EngineConfigs.h>
#include <omniscript/engine/Backends/JITBackend.h>

class JITCompiler {
private:
    std::shared_ptr<JITBackend> backend;

public:
    JITCompiler(std::shared_ptr<JITBackend> backend)
        : backend(std::move(backend)) {
        this->backend->initialize();
    }

    void execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config& config) {
        backend->execute(statements, config);
    }
};