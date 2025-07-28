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
    
    int startLine = std::max(1, span.start.line - contextLines);
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
}

void Console::reportError(ErrorType errorType, const std::string& message, 
                         const std::string& suggestion, const FileSpan& span) {
    reportError(errorType, message, span);
    
    if (!suggestion.empty()) {
        std::cout << getColorCode(HELP) << "help: " << "\033[0m" << suggestion << std::endl;
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