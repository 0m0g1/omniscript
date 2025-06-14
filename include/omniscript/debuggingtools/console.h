#pragma once

#ifdef DEBUG
#undef DEBUG
#endif

#include <future>
#include <cassert>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Core.h>


class Console {
public:
    enum LogLevel { LOG, INFO, WARN, ERR, DEBUG_LOG };

    // std::string toString(int indentLevel = 0) const override {
    //     return "console";
    // }

    // Singleton instance of Console
    static Console& instance() {
        static Console instance;
        return instance;
    }

    void enableDebug(bool state = true) { debugEnabled = state; }
    bool isDebugging() const {return debugEnabled; }

    void log(const std::string& message = "", bool addNewline = true, LogLevel level = LOG) {
        if (level == DEBUG_LOG && !debugEnabled) return;

        switch (level) {
            case LOG:
                std::cout << "\033[0;37m" << message << "\033[0m";  // Gray text
                break;
            case INFO:
                std::cout << "\033[1;32m" << message << "\033[0m"; // Cyan text
                break;
            case WARN:
                std::cout << "\033[1;33m" << message << "\033[0m"; // Yellow text
                break;
           case ERR: {
                Omniscript::filePosition position = Omniscript::getPosition();

                std::string errorMessage = 
                    "\033[1;31m" // Bold red text
                    "ERROR:\n"
                    "  File   : " + position.filePath + "\n" +
                    "  Line   : " + std::to_string(position.line) + "\n" +
                    "  Column : " + std::to_string(position.col) + "\n\n" +
                    message + "\033[0m\n\nPress Enter to terminate...\n";

                std::cerr << errorMessage;

                // Save to log file
                std::ofstream logFile("error_log.txt", std::ios::out | std::ios::trunc);
                if (logFile.is_open()) {
                    logFile << "ERROR:\n"
                            << "  File   : " << position.filePath << "\n"
                            << "  Line   : " << position.line << "\n"
                            << "  Column : " << position.col << "\n\n"
                            << message << "\n";
                    logFile.close();
                } else {
                    std::cerr << "Unable to open error_log.txt for writing.\n";
                }

                // Wait for enter, but allow timeout
                auto future = std::async(std::launch::async, [] {
                    return std::cin.get();
                });

                if (future.wait_for(std::chrono::seconds(4)) == std::future_status::ready) {
                    char c = future.get();
                    std::exit(EXIT_FAILURE);
                } else {
                    std::exit(EXIT_FAILURE);
                }

                break;
            }
            case DEBUG_LOG:
                std::cout << "\033[0;34m" << message << "\033[0m"; // Blue text
                break;
            default:
                throw std::runtime_error("Undefined log level for console: LOG, INFO, WARN, ERR, DEBUG_LOG");
        }

        if (addNewline) {
            std::cout << std::endl;
        }
    }

    void info(const std::string& message = "", bool addNewline = true) { log(message, addNewline, INFO); }
    void warn(const std::string& message = "", bool addNewline = true) { log(message, addNewline, WARN); }
    void error(const std::string& message = "", bool addNewline = true) { log(message, addNewline, ERR); }
    void debug(const std::string& message = "", bool addNewline = true) { 
        if (debugEnabled) { 
            log(message, addNewline, DEBUG_LOG);
        } 
        return;
    }

    // void assert(bool condition, const std::string& message) {
    //     if (!condition) {
    //         error("Assertion failed: " + message);
    //     }
    // }

    void time(const std::string& label) {
        timers[label] = std::chrono::high_resolution_clock::now();
    }

    void timeEnd(const std::string& label) {
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

    template <typename T>
    void table(const std::vector<T>& items, const std::vector<std::string>& headers) {
        if (headers.size() != 2) {
            error("Error: Header size mismatch!");
            return;
        }

        const int colWidth = 15; 

        std::cout << std::left;
        for (const auto& header : headers) {
            std::cout << std::setw(colWidth) << header;
        }

        std::cout << "\n";
        
        std::cout << std::string(colWidth * headers.size(), '-') << "\n";

        for (const auto& item : items) {
            std::cout << std::setw(colWidth) << item << "\n";
        }
    }

    void clear() {
        std::cout << "\033[2J\033[1;1H";  // ANSI escape codes to clear the console
        info("Console cleared");
    }

private:
    Console(bool debugEnabled = false) : debugEnabled(debugEnabled) {}

    bool debugEnabled;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> timers;
};

#define console Console::instance()