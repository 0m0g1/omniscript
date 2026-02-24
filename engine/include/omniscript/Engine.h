#pragma once

#include <expected>
#include <memory>
#include <expected>
#include <string>

#include <omniscript/EngineConfigs.h>

namespace Omniscript {

class Backend;

class Engine {
public:
    Engine(int argc, char** argv);

    int run();

private:
    int m_argc{};
    char** m_argv{};

    std::string readSourceFile(const std::string& file_path) const;

    // Parse CLI -> Config (single source of truth)
    static std::expected<Config, std::string> parseArguments(int argc, char** argv) noexcept;

    // Pick backend based on Config (LLVM default)
    static std::unique_ptr<Backend> makeBackend(const Config& config);
};

} // namespace Omniscript