# Console

## Purpose
The `Console` component is the central diagnostic and logging system in the OmniScript++ (OS) compiler. It provides a flexible, thread-safe interface for reporting errors, warnings, notes, and debugging information, complete with source location context via `FileSpan`. `Console` supports advanced features like colored output, performance timing, and source code context display, making it a cornerstone of user-friendly diagnostics. In professional compiler design, a robust diagnostic system is crucial for developer productivity, and `Console` exemplifies this with its comprehensive error reporting and macro-based utilities.

## Declarations
Below is the header file for `Console`, defining the `Console` class, enums, and debugging macros.

```cpp
#pragma once

#include <omniscript/FileSpan.h>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

namespace Omniscript {

class Console {
public:
    enum LogLevel {
        LOG,
        INFO,
        WARN,
        ERR,
        DEBUG_LOG,
        FATAL,
        NOTE,
        HELP
    };

    enum ErrorType {
        SYNTAX_ERROR,
        TYPE_ERROR,
        RUNTIME_ERROR,
        SEMANTIC_ERROR,
        PARSE_ERROR,
        LINK_ERROR,
        IO_ERROR,
        INTERNAL_ERROR,
        FATAL_ERROR
    };

    static Console& instance();

    void enableDebug(bool state = true);
    bool isDebugging() const;

    void log(const std::string& message = "", bool addNewline = true, LogLevel level = LOG);
    void info(const std::string& message = "", bool addNewline = true);
    void warn(const std::string& message = "", bool addNewline = true);
    void error(const std::string& message = "", bool addNewline = true);
    void debug(const std::string& message = "", bool addNewline = true);
    void fatal(const std::string& message = "", bool addNewline = true);
    void note(const std::string& message = "", bool addNewline = true);
    void help(const std::string& message = "", bool addNewline = true);

    void reportError(ErrorType errorType, const std::string& message, const FileSpan& span = getSpan());
    void reportError(ErrorType errorType, const std::string& message, const std::string& suggestion, const FileSpan& span = getSpan());
    void reportWarning(const std::string& message, const FileSpan& span = getSpan());
    void reportNote(const std::string& message, const FileSpan& span = getSpan());

    void reportErrorWithContext(ErrorType errorType, const std::string& message, const std::vector<std::pair<FileSpan, std::string>>& contexts);

    void showSourceContext(const FileSpan& span, int contextLines = 2);
    void showSourceLine(const std::string& filePath, int lineNumber, const std::string& marker = "");

    void beginDiagnostic(const std::string& phase);
    void endDiagnostic();
    void addDiagnosticNote(const std::string& note);

    void time(const std::string& label);
    void timeEnd(const std::string& label);

    template <typename T>
    void table(const std::vector<T>& items, const std::vector<std::string>& headers);

    void clear();
    void setColorEnabled(bool enabled);
    bool isColorEnabled() const;

    int getErrorCount() const;
    int getWarningCount() const;
    void resetCounts();

    static std::string formatString(const char* format, ...);

private:
    Console(bool debugEnabled = false);
    ~Console() = default;
    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    std::string getErrorTypeName(ErrorType type) const;
    std::string getColorCode(LogLevel level) const;
    std::string formatSpan(const FileSpan& span) const;
    std::string loadSourceLine(const std::string& filePath, int lineNumber) const;
    void writeToLogFile(const std::string& content) const;

    bool debugEnabled;
    bool colorEnabled = true;
    int errorCount = 0;
    int warningCount = 0;
    std::string currentDiagnosticPhase;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> timers;
};

template <typename T>
void Console::table(const std::vector<T>& items, const std::vector<std::string>& headers) {
    if (headers.size() != 2) {
        error("Error: Header size mismatch!");
        return;
    }

    const int colWidth = 15;

    std::cout << std::left;
    for (const auto& header : headers) {
        std::cout << std::setw(colWidth) << header;
    }

    std::cout << "\n" << std::string(colWidth * headers.size(), '-') << "\n";

    for (const auto& item : items) {
        std::cout << std::setw(colWidth) << item << "\n";
    }
}

}  // namespace Omniscript

#define console Omniscript::Console::instance()

// Debug macros
#ifdef DEBUG
    #define DEBUG_LOG(msg) console.debug(msg)
    #define DEBUG_LOG_SPAN(msg, span) console.debug(std::string(msg) + " at " + (span).toString())
    #define DEBUG_LOG_F(fmt, ...) console.debug(Console::formatString(fmt, __VA_ARGS__))
    #define DEBUG_TRACE() console.debug(std::string("TRACE: ") + __func__ + " (" + __FILE__ + ":" + std::to_string(__LINE__) + ")")
    #define DEBUG_VAR(var) console.debug(std::string(#var) + " = " + std::to_string(var))
    #define DEBUG_STR(var) console.debug(std::string(#var) + " = \"" + std::string(var) + "\"")
    #define DEBUG_SPAN(span) console.debug(std::string("Span: ") + (span).toString())
    #define DEBUG_PHASE(phase) console.beginDiagnostic(phase)
    #define DEBUG_PHASE_END() console.endDiagnostic()
    #define DEBUG_NOTE(note) console.addDiagnosticNote(note)
#else
    #define DEBUG_LOG(msg) ((void)0)
    #define DEBUG_LOG_SPAN(msg, span) ((void)0)
    #define DEBUG_LOG_F(fmt, ...) ((void)0)
    #define DEBUG_TRACE() ((void)0)
    #define DEBUG_VAR(var) ((void)0)
    #define DEBUG_STR(var) ((void)0)
    #define DEBUG_SPAN(span) ((void)0)
    #define DEBUG_PHASE(phase) ((void)0)
    #define DEBUG_PHASE_END() ((void)0)
    #define DEBUG_NOTE(note) ((void)0)
#endif

#define REPORT_ERROR(errorType, message) \
    console.reportError(errorType, \
        Console::formatString("%s (in %s at %s:%d)", \
            std::string(message).c_str(), __func__, __FILE__, __LINE__))

#define REPORT_ERROR_WITH_SUGGESTION(errorType, message, suggestion) \
    console.reportError(errorType, \
        Console::formatString("%s (in %s at %s:%d)", \
            std::string(message).c_str(), __func__, __FILE__, __LINE__), \
        suggestion)

#define REPORT_ERROR_WITH_SPAN(errorType, message, span) \
    console.reportError(errorType, \
        Console::formatString("%s (in %s at %s:%d)", \
            std::string(message).c_str(), __func__, __FILE__, __LINE__), \
        span)

#define REPORT_ERROR_WITH_SPAN_AND_SUGGESTION(errorType, message, suggestion, span) \
    console.reportError(errorType, \
        Console::formatString("%s (in %s at %s:%d)", \
            std::string(message).c_str(), __func__, __FILE__, __LINE__), \
        suggestion, span)

// Formatted error reporting with printf-style formatting
#define REPORT_ERROR_F(errorType, format, ...) \
    console.reportError(errorType, \
        Console::formatString("%s (in %s at %s:%d)", \
            Console::formatString(format, __VA_ARGS__).c_str(), \
            __func__, __FILE__, __LINE__))

#define REPORT_ERROR_F_WITH_SUGGESTION(errorType, format, suggestion, ...) \
    console.reportError(errorType, \
        Console::formatString("%s (in %s at %s:%d)", \
            Console::formatString(format, __VA_ARGS__).c_str(), \
            __func__, __FILE__, __LINE__), \
        suggestion)

// Warning macros with source location
#define REPORT_WARNING(message) \
    console.reportWarning( \
        Console::formatString("%s (in %s at %s:%d)", \
            std::string(message).c_str(), __func__, __FILE__, __LINE__))

#define REPORT_WARNING_F(format, ...) \
    console.reportWarning( \
        Console::formatString("%s (in %s at %s:%d)", \
            Console::formatString(format, __VA_ARGS__).c_str(), \
            __func__, __FILE__, __LINE__))

// Fatal error macro that shows source location before terminating
#define FATAL_ERROR(message) \
    do { \
        console.reportError(Console::ErrorType::FATAL_ERROR, \
            Console::formatString("FATAL: %s (in %s at %s:%d)", \
                std::string(message).c_str(), __func__, __FILE__, __LINE__)); \
        std::exit(EXIT_FAILURE); \
    } while(0)

#define FATAL_ERROR_F(format, ...) \
    do { \
        console.reportError(Console::ErrorType::FATAL_ERROR, \
            Console::formatString("FATAL: %s (in %s at %s:%d)", \
                Console::formatString(format, __VA_ARGS__).c_str(), \
                __func__, __FILE__, __LINE__)); \
        std::exit(EXIT_FAILURE); \
    } while(0)

// Assert-style macros for internal errors
#define ASSERT_OR_INTERNAL_ERROR(condition, message) \
    do { \
        if (!(condition)) { \
            console.reportError(Console::ErrorType::INTERNAL_ERROR, \
                Console::formatString("Assertion failed: %s (in %s at %s:%d)", \
                    std::string(message).c_str(), __func__, __FILE__, __LINE__)); \
        } \
    } while(0)

// Quick error reporting for common cases
#define SYNTAX_ERROR(message) REPORT_ERROR(Console::ErrorType::SYNTAX_ERROR, message)
#define TYPE_ERROR(message) REPORT_ERROR(Console::ErrorType::TYPE_ERROR, message)
#define RUNTIME_ERROR(message) REPORT_ERROR(Console::ErrorType::RUNTIME_ERROR, message)
#define SEMANTIC_ERROR(message) REPORT_ERROR(Console::ErrorType::SEMANTIC_ERROR, message)
#define PARSE_ERROR(message) REPORT_ERROR(Console::ErrorType::PARSE_ERROR, message)
#define LINK_ERROR(message) REPORT_ERROR(Console::ErrorType::LINK_ERROR, message)
#define IO_ERROR(message) REPORT_ERROR(Console::ErrorType::IO_ERROR, message)
#define INTERNAL_ERROR(message) REPORT_ERROR(Console::ErrorType::INTERNAL_ERROR, message)

// Formatted versions
#define SYNTAX_ERROR_F(format, ...) REPORT_ERROR_F(Console::ErrorType::SYNTAX_ERROR, format, __VA_ARGS__)
#define TYPE_ERROR_F(format, ...) REPORT_ERROR_F(Console::ErrorType::TYPE_ERROR, format, __VA_ARGS__)
#define RUNTIME_ERROR_F(format, ...) REPORT_ERROR_F(Console::ErrorType::RUNTIME_ERROR, format, __VA_ARGS__)
#define SEMANTIC_ERROR_F(format, ...) REPORT_ERROR_F(Console::ErrorType::SEMANTIC_ERROR, format, __VA_ARGS__)
#define PARSE_ERROR_F(format, ...) REPORT_ERROR_F(Console::ErrorType::PARSE_ERROR, format, __VA_ARGS__)
#define LINK_ERROR_F(format, ...) REPORT_ERROR_F(Console::ErrorType::LINK_ERROR, format, __VA_ARGS__)
#define IO_ERROR_F(format, ...) REPORT_ERROR_F(Console::ErrorType::IO_ERROR, format, __VA_ARGS__)
#define INTERNAL_ERROR_F(format, ...) REPORT_ERROR_F(Console::ErrorType::INTERNAL_ERROR, format, __VA_ARGS__)
```

### Explanation
- **`LogLevel` Enum**: Defines logging categories (e.g., `INFO`, `ERR`, `DEBUG_LOG`) for different message types, each associated with a color code for terminal output.
- **`ErrorType` Enum**: Categorizes compiler errors (e.g., `SYNTAX_ERROR`, `TYPE_ERROR`) for precise diagnostic reporting.
- **`Console` Class**: A singleton (via `instance()`) ensuring a single logging instance. Key methods include:
  - Logging: `log()`, `info()`, `warn()`, `error()`, etc., for various message types.
  - Error Reporting: `reportError()`, `reportWarning()`, `reportNote()`, with `FileSpan` integration for location context.
  - Source Display: `showSourceContext()`, `showSourceLine()` for showing code snippets with error markers.
  - Diagnostics: `beginDiagnostic()`, `endDiagnostic()`, `addDiagnosticNote()` for tracking compilation phases.
  - Timing: `time()`, `timeEnd()` for performance profiling.
  - Utility: `table()` for tabular output, `clear()` for console clearing, and `formatString()` for printf-style formatting.
- **Macros**: Extensive macros (e.g., `REPORT_ERROR`, `SYNTAX_ERROR`, `DEBUG_LOG`) simplify error reporting and debugging, embedding source file and line information.
- **Private Members**: Manage state like `debugEnabled`, `colorEnabled`, `errorCount`, `warningCount`, and `timers` for performance tracking.

## Definitions
Below is the implementation file for `Console`, defining the methods declared in the header.

```cpp
#include <omniscript/Console.h>
#include <cstdarg>
#include <fstream> 

namespace Omniscript {

std::string Console::formatString(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Get required size
    int size = vsnprintf(nullptr, 0, format, args);
    va_end(args);
    
    if (size <= 0) return "";
    
    // Format the string
    std::string result(size, '\0');
    va_start(args, format);
    vsnprintf(&result[0], size + 1, format, args);
    va_end(args);
    
    return result;
}

Console& Console::instance() {
    static Console instance;
    return instance;
}

void Console::enableDebug(bool state) { 
    debugEnabled = state; 
}

bool Console::isDebugging() const { 
    return debugEnabled; 
}

void Console::setColorEnabled(bool enabled) { 
    colorEnabled = enabled; 
}

bool Console::isColorEnabled() const { 
    return colorEnabled; 
}

int Console::getErrorCount() const { 
    return errorCount; 
}

int Console::getWarningCount() const { 
    return warningCount; 
}

void Console::resetCounts() { 
    errorCount = 0; 
    warningCount = 0; 
}

std::string Console::getColorCode(LogLevel level) const {
    if (!colorEnabled) return "";
    
    switch (level) {
        case LOG:       return "\033[0;37m";  // Gray
        case INFO:      return "\033[1;36m";  // Cyan
        case WARN:      return "\033[1;33m";  // Yellow
        case ERR:       return "\033[1;31m";  // Red
        case FATAL:     return "\033[1;41m";  // White on red background
        case DEBUG_LOG: return "\033[0;34m";  // Blue
        case NOTE:      return "\033[1;32m";  // Green
        case HELP:      return "\033[1;35m";  // Magenta
        default:        return "\033[0m";     // Reset
    }
}

std::string Console::getErrorTypeName(ErrorType type) const {
    switch (type) {
        case SYNTAX_ERROR:   return "syntax error";
        case TYPE_ERROR:     return "type error";
        case RUNTIME_ERROR:  return "runtime error";
        case SEMANTIC_ERROR: return "semantic error";
        case PARSE_ERROR:    return "parse error";
        case LINK_ERROR:     return "link error";
        case IO_ERROR:       return "I/O error";
        case INTERNAL_ERROR: return "internal error";
        default:             return "error";
    }
}

std::string Console::formatSpan(const FileSpan& span) const {
    if (!span.isValid()) {
        return "unknown location";
    }
    
    if (span.start.line == span.end.line) {
        if (span.start.col == span.end.col) {
            return span.start.filePath + ":" + std::to_string(span.start.line) + 
                   ":" + std::to_string(span.start.col);
        } else {
            return span.start.filePath + ":" + std::to_string(span.start.line) + 
                   ":" + std::to_string(span.start.col) + "-" + std::to_string(span.end.col);
        }
    } else {
        return span.start.filePath + ":" + std::to_string(span.start.line) + 
               ":" + std::to_string(span.start.col) + " to " + 
               std::to_string(span.end.line) + ":" + std::to_string(span.end.col);
    }
}

void Console::log(const std::string& message, bool addNewline, LogLevel level) {
    if (level == DEBUG_LOG && !debugEnabled) return;

    std::string colorCode = getColorCode(level);
    std::string resetCode = colorEnabled ? "\033[0m" : "";
    
    std::cout << colorCode << message << resetCode;
    
    if (addNewline) {
        std::cout << std::endl;
    }
}

void Console::info(const std::string& message, bool addNewline) { 
    log(message, addNewline, INFO); 
}

void Console::warn(const std::string& message, bool addNewline) { 
    log(message, addNewline, WARN); 
}

void Console::error(const std::string& message, bool addNewline) { 
    log(message, addNewline, ERR); 
}

void Console::fatal(const std::string& message, bool addNewline) { 
    log(message, addNewline, FATAL); 
}

void Console::debug(const std::string& message, bool addNewline) { 
    if (debugEnabled) { 
        log(message, addNewline, DEBUG_LOG);
    }
}

void Console::note(const std::string& message, bool addNewline) { 
    log(message, addNewline, NOTE); 
}

void Console::help(const std::string& message, bool addNewline) { 
    log(message, addNewline, HELP); 
}

std::string Console::loadSourceLine(const std::string& filePath, int lineNumber) const {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return "<source unavailable>";
    }
    
    std::string line;
    int currentLine = 1;
    
    while (std::getline(file, line) && currentLine <= lineNumber) {
        if (currentLine == lineNumber) {
            return line;
        }
        currentLine++;
    }
    
    return "<line not found>";
}

void Console::showSourceLine(const std::string& filePath, int lineNumber, const std::string& marker) {
    std::string line = loadSourceLine(filePath, lineNumber);
    
    // Show line number and source
    std::cout << getColorCode(INFO) << std::setw(4) << lineNumber << " | " 
              << "\033[0m" << line << std::endl;
    
    // Show marker if provided
    if (!marker.empty()) {
        std::cout << "     | " << getColorCode(ERR) << marker << "\033[0m" << std::endl;
    }
}

void Console::showSourceContext(const FileSpan& span, int contextLines) {
    if (!span.isValid()) return;
    
    int startLine = std::max(1, static_cast<int>(span.start.line - contextLines));
    int endLine = span.end.line + contextLines;
    
    for (int i = startLine; i <= endLine; i++) {
        std::string line = loadSourceLine(span.start.filePath, i);
        
        // Highlight the error line(s)
        if (i >= span.start.line && i <= span.end.line) {
            std::cout << getColorCode(ERR) << std::setw(4) << i << " | " 
                      << "\033[0m" << line << std::endl;
            
            // Show caret indicators
            if (i == span.start.line) {
                std::string caret(5, ' ');
                caret += "| ";
                
                int startCol = span.start.col;
                int endCol = (i == span.end.line) ? span.end.col : line.length();
                
                for (int j = 0; j < startCol; j++) {
                    caret += (line[j] == '\t') ? '\t' : ' ';
                }
                
                caret += getColorCode(ERR);
                for (int j = startCol; j <= endCol && j < line.length(); j++) {
                    caret += '^';
                }
                caret += "\033[0m";
                
                std::cout << caret << std::endl;
            }
        } else {
            std::cout << getColorCode(INFO) << std::setw(4) << i << " | " 
                      << "\033[0m" << line << std::endl;
        }
    }
}

void Console::reportError(ErrorType errorType, const std::string& message, const FileSpan& span) {
    errorCount++;
    
    std::string location = formatSpan(span);
    std::cout << getColorCode(ERR) << getErrorTypeName(errorType) << ": " 
              << "\033[0m" << message << std::endl;
    
    if (span.isValid()) {
        std::cout << getColorCode(INFO) << "  --> " << location << "\033[0m" << std::endl;
        showSourceContext(span, 1);
    }
    
    writeToLogFile(getErrorTypeName(errorType) + " at " + location + ": " + message);

    if (errorType == RUNTIME_ERROR || errorType == FATAL_ERROR || errorType == INTERNAL_ERROR) {
        std::exit(EXIT_FAILURE);
    }
}

void Console::reportError(ErrorType errorType, const std::string& message, 
                         const std::string& suggestion, const FileSpan& span) {
    reportError(errorType, message, span);
    
    if (!suggestion.empty()) {
        std::cout << getColorCode(HELP) << "help: " << "\033[0m" << suggestion << std::endl;
    }

    if (errorType == RUNTIME_ERROR || errorType == FATAL_ERROR || errorType == INTERNAL_ERROR) {
        std::exit(EXIT_FAILURE);
    }
}

void Console::reportWarning(const std::string& message, const FileSpan& span) {
    warningCount++;
    
    std::string location = formatSpan(span);
    std::cout << getColorCode(WARN) << "warning: " << "\033[0m" << message << std::endl;
    
    if (span.isValid()) {
        std::cout << getColorCode(INFO) << "  --> " << location << "\033[0m" << std::endl;
        showSourceContext(span, 1);
    }
}

void Console::reportNote(const std::string& message, const FileSpan& span) {
    std::string location = formatSpan(span);
    std::cout << getColorCode(NOTE) << "note: " << "\033[0m" << message;
    
    if (span.isValid()) {
        std::cout << " at " << location;
    }
    std::cout << std::endl;
}

void Console::reportErrorWithContext(ErrorType errorType, const std::string& message, 
                                    const std::vector<std::pair<FileSpan, std::string>>& contexts) {
    errorCount++;
    
    std::cout << getColorCode(ERR) << getErrorTypeName(errorType) << ": " 
              << "\033[0m" << message << std::endl;
    
    for (const auto& context : contexts) {
        std::string location = formatSpan(context.first);
        std::cout << getColorCode(INFO) << "  --> " << location << "\033[0m";
        
        if (!context.second.empty()) {
            std::cout << ": " << context.second;
        }
        std::cout << std::endl;
        
        if (context.first.isValid()) {
            showSourceContext(context.first, 1);
        }
    }
}

void Console::beginDiagnostic(const std::string& phase) {
    currentDiagnosticPhase = phase;
    if (debugEnabled) {
        debug("Begin " + phase);
    }
}

void Console::endDiagnostic() {
    if (debugEnabled && !currentDiagnosticPhase.empty()) {
        debug("End " + currentDiagnosticPhase);
    }
    currentDiagnosticPhase.clear();
}

void Console::addDiagnosticNote(const std::string& note) {
    if (debugEnabled) {
        debug("  " + note);
    }
}

void Console::writeToLogFile(const std::string& content) const {
    std::ofstream logFile("omniscript_errors.log", std::ios::app);
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        logFile << "[" << std::ctime(&time_t);
        logFile.seekp(-1, std::ios_base::cur); // Remove newline
        logFile << "] " << content << std::endl;
        logFile.close();
    }
}

void Console::time(const std::string& label) {
    timers[label] = std::chrono::high_resolution_clock::now();
}

void Console::timeEnd(const std::string& label) {
    auto end = std::chrono::high_resolution_clock::now();
    if (timers.find(label) != timers.end()) {
        auto start = timers[label];
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        info(label + ": " + std::to_string(duration) + "ms");
        timers.erase(label);
    } else {
        warn("Timer \"" + label + "\" does not exist.");
    }
}

void Console::clear() {
    std::cout << "\033[2J\033[1;1H";
    info("Console cleared");
}

Console::Console(bool debugEnabled) : debugEnabled(debugEnabled) {}

} // namespace Omniscript
```

### Explanation
- **`formatString()`**: Implements printf-style formatting using `vsnprintf` for safe string construction.
- **`instance()`**: Returns a static singleton instance, ensuring thread-safe access (though not explicitly locked).
- **Logging Methods**: `log()` is the core output function, applying color codes and handling newlines. Specialized methods (`info()`, `warn()`, etc.) call `log()` with appropriate `LogLevel` values.
- **Error Reporting**: `reportError()` and variants increment `errorCount`, format messages with `FileSpan` locations, and display source context. Fatal errors trigger `std::exit()`.
- **Source Display**: `loadSourceLine()` reads specific lines from files, while `showSourceContext()` and `showSourceLine()` format code snippets with line numbers and error markers.
- **Diagnostics and Timing**: `beginDiagnostic()`/`endDiagnostic()` track compilation phases, and `time()`/`timeEnd()` measure performance.
- **Utility**: `writeToLogFile()` appends errors to a log file with timestamps, and `clear()` resets the terminal.
- **State Management**: Methods like `enableDebug()`, `setColorEnabled()`, and `resetCounts()` manage console state.

## Usage in OS Compiler
The `Console` is used extensively for diagnostic output. For example, in the `starfield` example (`examples/starfield.os`):

```os
let sprite: Sprite = null; // Invalid initialization
```

The type checker might invoke:

```cpp
SYNTAX_ERROR_F("Invalid initialization of Sprite at %s", getSpan().toString().c_str());
```

This produces output like:

```
syntax error: Invalid initialization of Sprite at starfield.os:3:14
  --> starfield.os:3:14-18
    3 | let sprite: Sprite = null;
                  ^^^^
```

The `Console`’s macros (`SYNTAX_ERROR`, `REPORT_WARNING`, etc.) are used across the parser, type checker, and code generator to ensure consistent, location-aware diagnostics.

## Development Notes
The `Console` was developed after `FileSpan` to leverage its source tracking capabilities. Design decisions include:
- **Singleton Pattern**: Ensures a single logging instance, simplifying global access via the `console` macro.
- **Color-Coded Output**: Enhances readability, with an option to disable colors for non-terminal environments.
- **Macro-Based Interface**: Macros like `SYNTAX_ERROR` embed file and line information, reducing boilerplate in other components.
- **Log File**: Persists errors for post-mortem analysis, a critical feature for debugging complex compilation issues.
Challenges included ensuring thread-safety (not fully addressed, as the singleton lacks mutex protection) and handling file I/O in `loadSourceLine()` robustly. The extensive macro system was iteratively refined to balance flexibility and simplicity.

## Dependencies
- **`FileSpan`**: Used for source location tracking in error reports and context display.
- **Standard Library**: Uses `<unordered_map>`, `<chrono>`, `<iomanip>`, `<iostream>`, `<string>`, `<vector>`, `<utility>`, `<cstdarg>`, and `<fstream>` for various utilities.
- **No Other OS Components**: `Console` is a standalone diagnostic system, though it interacts with components like the parser via `FileSpan`.

## Source Code
- Header: [https://github.com/0m0g1/omniscript/blob/main/include/omniscript/Console.h](https://github.com/0m0g1/omniscript/blob/main/include/omniscript/Console.h)
- Implementation: [https://github.com/0m0g1/omniscript/blob/main/src/Console.cpp](https://github.com/0m0g1/omniscript/blob/main/src/Console.cpp)

## Integration with Project
- **File Placement**:
  - Header: `include/omniscript/Console.h`
  - Implementation: `src/Console.cpp`
- **Build System**: The `premake5.lua` script includes `Console.cpp` in the source file list. The header depends on `FileSpan.h`, which must be in the include path. No special build flags are required.
- **Compatibility**: Fully compatible with the OS build system, supporting Debug/Release modes and LLVM integration.

## Adding to the Index
Add the following entry to `index.md` under the Component Reference table:

```markdown
| Console | Logging and diagnostics for error reporting. | [Console](Console.md) |
```