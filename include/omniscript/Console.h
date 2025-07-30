#pragma once

#ifdef DEBUG
#undef DEBUG
#endif

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