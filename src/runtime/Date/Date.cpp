#include <omniscript/runtime/Date/Date.h>
#include <omniscript/omniscript_pch.h>

Date::Date() {
    // Get the current time
    currentTime = std::time(nullptr);
    localTime = *std::localtime(&currentTime);

    // Register methods and properties
    registerMethods();
    registerProperties();
}

void Date::registerMethods() {
    addMethod("getYear", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return getYear();
    });

    addMethod("getMonth", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return getMonth();
    });

    addMethod("getDate", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return getDate();
    });

    addMethod("getDay", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return getDay();
    });

    addMethod("getHours", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return getHours();
    });

    addMethod("getMinutes", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return getMinutes();
    });

    addMethod("getSeconds", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return getSeconds();
    });

    addMethod("toString", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return getCurrentTime();
    });
}

void Date::registerProperties() {
    // No constant properties for Date, but dynamic getters can be added if needed
}

std::string Date::getCurrentTime() {
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

int Date::getYear() {
    return localTime.tm_year + 1900; // tm_year is years since 1900
}

int Date::getMonth() {
    return localTime.tm_mon; // tm_mon is 0-based (0 = January, 11 = December)
}

int Date::getDate() {
    return localTime.tm_mday; // tm_mday is day of the month
}

int Date::getDay() {
    return localTime.tm_wday; // tm_wday is day of the week (0 = Sunday, 6 = Saturday)
}

int Date::getHours() {
    return localTime.tm_hour; // tm_hour is hours since midnight
}

int Date::getMinutes() {
    return localTime.tm_min; // tm_min is minutes after the hour
}

int Date::getSeconds() {
    return localTime.tm_sec; // tm_sec is seconds after the minute
}