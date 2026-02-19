#ifndef OS_H
#define OS_H

#include <omniscript/omniscript_pch.h>
#include <omniscript/runtime/object.h>

class OS : public Object {
public:
    OS();

    // Register methods and properties
    void registerMethods();
    void registerProperties();

    // System operations
    std::string getPlatform();
    void sleep(int milliseconds);
    void execute(const std::string& command);
    std::vector<std::string> getEnvironmentVariables();

private:
    std::string platform;
};

#endif // OS_H
