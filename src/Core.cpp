// core.cpp
#include <omniscript/Core.h>
#include <omniscript/FileSpan.h>

namespace Omniscript {
// StringInterner implementation
namespace detail {
    StringInterner globalInterner;
    std::string_view StringInterner::intern(std::string_view str) noexcept {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto [it, inserted] = strings_.emplace(str);
        return *it;
    }
    
    void StringInterner::clear() noexcept {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        strings_.clear();
    }
    
    constexpr bool fast_string_equal(std::string_view a, std::string_view b) noexcept {
        return a == b; // Optimize with SIMD later if needed
    }
}

// ErrorCollector implementation  
namespace error {
    ErrorCollector globalErrorCollector;
    void ErrorCollector::addError(Severity severity, std::string message, std::string context) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        errors_.emplace_back(ErrorInfo{
            severity,
            std::move(message),
            std::move(context),
            std::chrono::system_clock::now(),
            std::this_thread::get_id()
        });
    }
    
    bool ErrorCollector::hasErrors() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return !errors_.empty();
    }
    
    bool ErrorCollector::hasFatalErrors() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::any_of(errors_.begin(), errors_.end(),
            [](const ErrorInfo& err) { return err.severity == Severity::Fatal; });
    }
    
    const std::vector<ErrorInfo>& ErrorCollector::getErrors() const noexcept {
        // Note: This is not thread-safe as written, consider returning a copy
        return errors_;
    }
    
    void ErrorCollector::clear() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        errors_.clear();
    }
}

// MemoryTracker implementation
namespace perf {
    MemoryTracker::MemoryTracker() noexcept : baseline_(getCurrentUsage()) {
        peak_.store(baseline_, std::memory_order_relaxed);
    }
    
    MemoryTracker::~MemoryTracker() noexcept = default;
    
    size_t MemoryTracker::getCurrentUsage() const noexcept {
        // Platform-specific memory usage implementation
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize;
        }
#elif __linux__
        std::ifstream status("/proc/self/status");
        std::string line;
        while (std::getline(status, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                return std::stoull(line.substr(7)) * 1024; // Convert KB to bytes
            }
        }
#endif
        return 0;
    }
    
    size_t MemoryTracker::getPeakUsage() const noexcept {
        auto current = getCurrentUsage();
        auto current_peak = peak_.load(std::memory_order_relaxed);
        
        while (current > current_peak) {
            if (peak_.compare_exchange_weak(current_peak, current, std::memory_order_relaxed)) {
                break;
            }
        }
        
        return peak_.load(std::memory_order_relaxed);
    }
    
    void MemoryTracker::reset() noexcept {
        baseline_ = getCurrentUsage();
        peak_.store(baseline_, std::memory_order_relaxed);
    }
}

}
