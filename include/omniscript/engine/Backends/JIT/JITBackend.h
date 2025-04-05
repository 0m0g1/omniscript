#pragma once
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/EngineConfigs.h>

class JITBackend {
public:
    virtual ~JITBackend() = default;

    virtual void initialize() = 0;

    virtual void execute(
        const std::vector<std::shared_ptr<Statement>>& statements,
        const Config& config
    ) = 0;
};



class DummyJITBackend : public JITBackend {
public:
    void initialize() override {
        // No-op
    }

    void execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config& config) override {
        std::cout << "[DummyJIT] Simulating execution of " << statements.size() << " statements.\n";
    }
};