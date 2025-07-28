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
        INTERNAL_ERROR
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
