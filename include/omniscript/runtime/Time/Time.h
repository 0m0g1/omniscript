#ifndef TIME_H
#define TIME_H

#include <omniscript/runtime/object.h>
#include <omniscript/omniscript_pch.h>

class Time : public Object {
public:
    Time();


private:
    void registerMethods();
    void registerProperties();
    // Helper methods
    SymbolTable::ValueType getCurrentTime();
    SymbolTable::ValueType formatTime(time_t timestamp, const std::string& formatStr);
};

#endif // TIME_H
