#pragma once
#include <omniscript/omniscript_pch.h>
#include <omniscript/Backend.h>

namespace Omniscript {
class JITBackend : public Backend {
public:
    virtual ~JITBackend() = default;
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

} // namespace Omniscript
