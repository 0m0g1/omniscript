# Core

## Purpose
The `Core` component in the OmniScript++ (OS) compiler provides foundational utilities for performance optimization, memory management, and error handling. It includes `StringInterner` for memory-efficient string storage, `ScopedTimer` for performance profiling, `MemoryTracker` for monitoring memory usage, and `ErrorCollector` for thread-safe error aggregation. These utilities are critical for building a professional-grade compiler, enabling efficient resource use, precise diagnostics, and robust error management. The `Core` component underpins other compiler stages, such as parsing and code generation, by providing tools to optimize and debug the compilation process.

## Declarations
Below is the header file for `Core`, defining the utility classes, namespaces, and macros.

```cpp
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
```

### Explanation
- **`detail` Namespace**:
  - `fast_string_equal()`: A placeholder for optimized string comparison, intended for SIMD enhancements.
  - `is_valid_enum_value()`: A template for compile-time enum validation, not implemented in the provided code.
  - `StringInterner`: Interns strings to reduce memory usage by storing unique instances. Uses a `shared_mutex` for thread-safe access.
  - `globalInterner`: A global instance for string interning across the compiler.
- **`perf` Namespace**:
  - `ScopedTimer`: Measures execution time of a scope, storing duration in a provided `milliseconds` reference.
  - `MemoryTracker`: Tracks process memory usage, supporting current and peak measurements with platform-specific implementations.
- **`error` Namespace**:
  - `Severity` Enum: Categorizes diagnostic messages (Info, Warning, Error, Fatal).
  - `ErrorInfo` Struct: Stores error details, including severity, message, context, timestamp, and thread ID.
  - `ErrorCollector`: Aggregates errors in a thread-safe manner using a mutex-protected vector.
  - `globalErrorCollector`: A global instance for error collection.
- **Macros**:
  - Performance: `OMNISCRIPT_LIKELY`, `OMNISCRIPT_UNLIKELY`, `OMNISCRIPT_COLD`, `OMNISCRIPT_HOT`, `OMNISCRIPT_FORCE_INLINE` optimize branch prediction and inlining.
  - Profiling: `OMNISCRIPT_PROFILE_SCOPE` and `OMNISCRIPT_PROFILE_FUNCTION` enable timing in profiling builds, disabled in release mode.

## Definitions
Below is the implementation file for `Core`, defining the methods declared in the header.

```cpp
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

#include <fstream>  // Needed for Linux memory reading
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <algorithm>

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
```

### Explanation
- **`detail::StringInterner`**:
  - `intern()`: Adds a string to the set, returning a view to the stored instance. Uses `unique_lock` for exclusive write access.
  - `clear()`: Removes all interned strings, also under exclusive lock.
- **`detail::fast_string_equal()`**: A simple equality check, with a comment indicating future SIMD optimization.
- **`error::ErrorCollector`**:
  - `addError()`: Appends an `ErrorInfo` struct with thread ID and timestamp, protected by a mutex.
  - `hasErrors()`/`hasFatalErrors()`: Check error presence or fatal errors, thread-safely.
  - `getErrors()`: Returns the error vector (noted as non-thread-safe in the code).
  - `clear()`: Resets the error list.
- **`perf::MemoryTracker`**:
  - Constructor: Initializes `baseline_` and `peak_` with current memory usage.
  - `getCurrentUsage()`: Retrieves process memory usage via platform-specific APIs (`GetProcessMemoryInfo` on Windows, `/proc/self/status` on Linux).
  - `getPeakUsage()`: Updates and returns the peak memory usage using atomic operations for thread-safety.
  - `reset()`: Resets baseline and peak to current usage.
- **`perf::ScopedTimer`**: Implemented inline in the header, measures duration between construction and destruction.

## Usage in OS Compiler
The `Core` utilities are used across the OS compiler to optimize and debug compilation. Examples include:
- **StringInterner**: In the parser, identifiers like `Vec2` or `Sprite` in `examples/types.os` are interned to reduce memory usage:
  ```cpp
  auto ident = detail::globalInterner.intern(token.text);
  ```
  This ensures repeated identifiers share memory.
- **ScopedTimer**: Profiles parsing performance:
  ```cpp
  std::chrono::milliseconds parse_time;
  {
      OMNISCRIPT_PROFILE_SCOPE(parse_time);
      parse_program();
  }
  console.info("Parsing took " + std::to_string(parse_time.count()) + "ms");
  ```
- **MemoryTracker**: Monitors memory during code generation:
  ```cpp
  perf::MemoryTracker tracker;
  generate_llvm_ir();
  console.info("Peak memory: " + std::to_string(tracker.getPeakUsage() / 1024) + "KB");
  ```
- **ErrorCollector**: Aggregates errors during semantic analysis:
  ```cpp
  error::globalErrorCollector.addError(error::Severity::Error, "Type mismatch", getSpan().toString());
  if (globalErrorCollector.hasFatalErrors()) std::exit(1);
  ```

These utilities enhance the compiler’s efficiency and diagnostic capabilities, critical for handling complex OS scripts like the `starfield` example.

## Development Notes
The `Core` component was developed early to provide foundational tools for the OS compiler. Key design decisions include:
- **Thread-Safety**: `StringInterner` uses `shared_mutex` for read-heavy access, while `ErrorCollector` uses `mutex` for simpler write operations. `MemoryTracker` uses atomics for peak tracking.
- **Platform-Specific Code**: `MemoryTracker` handles Windows and Linux memory APIs, with fallback to zero for unsupported platforms.
- **Profiling Macros**: Conditional compilation ensures zero overhead in release builds.
Challenges included balancing thread-safety with performance in `StringInterner` (resolved with `shared_mutex`) and ensuring portable memory tracking (addressed with platform-specific code). The unimplemented `is_valid_enum_value()` suggests planned compile-time validation, deferred for later development. The `Core` component was a prerequisite for performance-sensitive components like the parser and code generator.

## Dependencies
- **`FileSpan`**: Used by `ErrorCollector` for context strings (e.g., source locations).
- **`Console`**: Used indirectly for reporting profiled data or errors (e.g., via `console.info()`).
- **Standard Library**: Uses `<string>`, `<string_view>`, `<unordered_set>`, `<shared_mutex>`, `<mutex>`, `<vector>`, `<chrono>`, `<thread>`, `<atomic>`, `<fstream>`, `<sstream>`, `<algorithm>` for various utilities.
- **Platform-Specific**: On Windows, requires `<windows.h>` and `<psapi.h>`; on Linux, uses `/proc/self/status` via `<fstream>`.

## Source Code
- Header: [https://github.com/0m0g1/omniscript/blob/main/include/omniscript/Core.h](https://github.com/0m0g1/omniscript/blob/main/include/omniscript/Core.h)
- Implementation: [https://github.com/0m0g1/omniscript/blob/main/src/Core.cpp](https://github.com/0m0g1/omniscript/blob/main/src/Core.cpp)

## Integration with Project
- **File Placement**:
  - Header: `include/omniscript/Core.h`
  - Implementation: `src/Core.cpp`
- **Build System**: The `premake5.lua` script includes `Core.cpp` in the source file list. The header depends on `FileSpan.h` and `Console.h`, which must be in the include path. On Windows, links against `psapi.lib` for memory tracking.
- **Compatibility**: Supports Debug/Release modes and integrates with the OS build system, including LLVM dependencies. Platform-specific code ensures portability.

## Adding to the Index
Add the following entry to your `index.md` under the Component Reference table:

```markdown
| Core | Foundational utilities for string interning, performance profiling, and error handling. | [Core](Core.md) |
```