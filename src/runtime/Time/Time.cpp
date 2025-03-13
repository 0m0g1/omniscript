#include <omniscript/runtime/object.h>
#include <omniscript/runtime/Time/Time.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/debuggingtools/console.h>

// Constructor
Time::Time() {
    registerMethods();
    registerProperties();
}

// Register methods
void Time::registerMethods() {
    addMethod("now", [this](const ArgumentDefinition& args) {
        return getCurrentTime();
    });

    addMethod("format", [this](const ArgumentDefinition& args) {
        if (args.size() != 2) {
            throw std::runtime_error("format() expects exactly 2 arguments: timestamp and format string.");
        }
        if (!std::holds_alternative<long long>(args[0]) || !std::holds_alternative<std::string>(args[1])) {
            console.error("Time.format() takes in two arguments a long long and string");
        }
        int timestamp = std::get<long long>(args[0]);
        std::string formatStr = std::get<std::string>(args[1]);

        return formatTime(static_cast<time_t>(timestamp), formatStr);
    });

    addMethod("sleep", [this](const ArgumentDefinition& args) {
        if (args.size() != 1) {
            throw std::runtime_error("sleep() expects 1 argument: duration in milliseconds.");
        }
        if (!std::holds_alternative<int>(args[0])) {
            console.error("Time.sleep() takes in one agruement an int");
        }

        int duration = std::get<int>(args[0]);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(duration)));
        return SymbolTable::ValueType{};
    });
}

// Register properties
void Time::registerProperties() {
    // setProperty("timestamp", [this]() { return getCurrentTime(); });
}

// Helper methods
SymbolTable::ValueType Time::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    return epoch;
}

SymbolTable::ValueType Time::formatTime(time_t timestamp, const std::string& formatStr) {
    std::ostringstream oss;
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &timestamp);
#else
    localtime_r(&timestamp, &tm);
#endif
    oss << std::put_time(&tm, formatStr.c_str());
    return oss.str();
}
