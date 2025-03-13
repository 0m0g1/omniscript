#ifndef DATE_H
#define DATE_H

#include <omniscript/omniscript_pch.h>
#include <omniscript/runtime/object.h>

class Date : public Object {
public:
    Date();

    // Register methods and properties
    void registerMethods();
    void registerProperties();

    // Helper methods for date and time calculations
    std::string getCurrentTime();
    int getYear();
    int getMonth(); // 0-based, like in JavaScript
    int getDate();  // Day of the month
    int getDay();   // Day of the week (0=Sunday, 6=Saturday)
    int getHours();
    int getMinutes();
    int getSeconds();

private:
    std::time_t currentTime;
    std::tm localTime;
};

#endif // DATE_H