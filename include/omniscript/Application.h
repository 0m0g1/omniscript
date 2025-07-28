#pragma once

#include <omniscript/Engine.h>
#include <atomic>
#include <chrono>
#include <expected>

namespace Omniscript {

class Application final {
public:
    explicit Application(int argc, char* argv[]) noexcept;
    ~Application() noexcept;
    
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;
    
    [[nodiscard]] int run() noexcept;

    static void signalHandler(int signal);

private:
    void initializeGlobalState() noexcept;
    void cleanupGlobalState() noexcept;
    void setupSignalHandlers() noexcept;
    void setupMemoryLimits() noexcept;
    void displayProfilerResults(const Config& config) noexcept;
    void logErrors(const Config& config) noexcept;
    
    std::expected<Config, std::string> config_;
    static std::atomic<bool> shutdownRequested_;
    std::chrono::steady_clock::time_point startTime_;
};

} // namespace Omniscript