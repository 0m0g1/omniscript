#pragma once

#include <string>
#include <string_view>
#include <unordered_set>
#include <shared_mutex>
#include <mutex>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>

#include <omniscript/FileSpan.h>
#include <omniscript/Console.h>

namespace Omniscript {
// Compile-time utilities
namespace detail {
    // Fast string comparison using SIMD when available
    [[nodiscard]] constexpr bool fast_string_equal(std::string_view a, std::string_view b) noexcept;
    
    // Compile-time argument validation
    template<typename T>
    [[nodiscard]] constexpr bool is_valid_enum_value(T value) noexcept;
    
    // Memory-efficient string interning for repeated strings
    class StringInterner {
    public:
        [[nodiscard]] std::string_view intern(std::string_view str) noexcept;
        void clear() noexcept;
        
    private:
        std::unordered_set<std::string> strings_;
        mutable std::shared_mutex mutex_;
    };
    
    extern StringInterner globalInterner;
}

// Performance monitoring utilities
namespace perf {
    class ScopedTimer {
    public:
        explicit ScopedTimer(std::chrono::milliseconds& target) noexcept 
            : target_(target), start_(std::chrono::steady_clock::now()) {}
        
        ~ScopedTimer() noexcept {
            auto end = std::chrono::steady_clock::now();
            target_ = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
        }
        
    private:
        std::chrono::milliseconds& target_;
        std::chrono::steady_clock::time_point start_;
    };
    
    class MemoryTracker {
    public:
        MemoryTracker() noexcept;
        ~MemoryTracker() noexcept;
        
        [[nodiscard]] size_t getCurrentUsage() const noexcept;
        [[nodiscard]] size_t getPeakUsage() const noexcept;
        void reset() noexcept;
        
    private:
        size_t baseline_;
        mutable std::atomic<size_t> peak_{0};
    };
}

// Error handling utilities
namespace error {
    enum class Severity {
        Info = 0,
        Warning = 1, 
        Error = 2,
        Fatal = 3
    };
    
    struct ErrorInfo {
        Severity severity;
        std::string message;
        std::string context;
        std::chrono::system_clock::time_point timestamp;
        std::thread::id threadId;
    };
    
    class ErrorCollector {
    public:
        void addError(Severity severity, std::string message, std::string context = {}) noexcept;
        [[nodiscard]] bool hasErrors() const noexcept;
        [[nodiscard]] bool hasFatalErrors() const noexcept;
        [[nodiscard]] const std::vector<ErrorInfo>& getErrors() const noexcept;
        void clear() noexcept;
        
    private:
        mutable std::mutex mutex_;
        std::vector<ErrorInfo> errors_;
    };
    
    extern ErrorCollector globalErrorCollector;
}

} // namespace Omniscript

// Convenience macros for performance-critical sections
#define OMNISCRIPT_LIKELY [[likely]]
#define OMNISCRIPT_UNLIKELY [[unlikely]]
#define OMNISCRIPT_COLD [[gnu::cold]]
#define OMNISCRIPT_HOT [[gnu::hot]]
#define OMNISCRIPT_FORCE_INLINE [[gnu::always_inline]] inline

// Profiling macros (compiled out in release builds)
#ifdef OMNISCRIPT_ENABLE_PROFILING
    #define OMNISCRIPT_PROFILE_SCOPE(name) ::perf::ScopedTimer _timer_##__LINE__(name)
    #define OMNISCRIPT_PROFILE_FUNCTION() OMNISCRIPT_PROFILE_SCOPE(__func__)
#else
    #define OMNISCRIPT_PROFILE_SCOPE(name) ((void)0)
    #define OMNISCRIPT_PROFILE_FUNCTION() ((void)0)
#endif