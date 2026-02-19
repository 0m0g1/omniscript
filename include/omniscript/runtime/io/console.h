#pragma once

#ifdef DEBUG_LOG
#undef DEBUG_LOG
#endif

#include <omniscript/omniscript_pch.h>
#include <omniscript/runtime/object.h>

class ConsoleObject : public Object {
public:
    ConsoleObject();
    std::string toString(int indentLevel = 0) const override {
        return "[console]";
    }

private:
    bool debugEnabled = false;
    enum LogLevel { LOG, INFO, WARN, ERR, DEBUG_LOG };
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> timers;


    void log(const ArgumentDefinition& args, LogLevel level);
    void info(const ArgumentDefinition& args);
    void warn(const ArgumentDefinition& args);
    void error(const ArgumentDefinition& args);
    void debug(const ArgumentDefinition& args);

    void assertCondition(const ArgumentDefinition& args);
    void startTime(const ArgumentDefinition& args);
    void endTime(const ArgumentDefinition& args);
    void displayTable(const ArgumentDefinition& args);
    // void clearConsole(const ArgumentDefinition& args);
    SymbolTable::ValueType input(const ArgumentDefinition& args); // string
    void pause(const ArgumentDefinition& args);
    void logMemoryUsage(const ArgumentDefinition& args);
};