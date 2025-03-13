#include <omniscript/runtime/OS/OS.h>
#include <omniscript/omniscript_pch.h>

OS::OS() {
    // Detect the current platform
    #ifdef _WIN32
        platform = "Windows";
    #elif __APPLE__
        platform = "MacOS";
    #elif __linux__
        platform = "Linux";
    #else
        platform = "Unknown";
    #endif

    // Register methods and properties
    registerMethods();
    registerProperties();
}

void OS::registerMethods() {
    addMethod("getPlatform", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return getPlatform();
    });

    addMethod("sleep", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1 && std::holds_alternative<int>(args[0])) {
            sleep(std::get<int>(args[0]));
        }
        return SymbolTable::ValueType();
    });

    addMethod("execute", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1 && std::holds_alternative<std::string>(args[0])) {
            execute(std::get<std::string>(args[0]));
        }
        return SymbolTable::ValueType();
    });

    addMethod("getEnvironmentVariables", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return getEnvironmentVariables();
    });
}

void OS::registerProperties() {
    // Register platform as a property if needed
}

std::string OS::getPlatform() {
    return platform;
}

void OS::sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void OS::execute(const std::string& command) {
    std::system(command.c_str());
}

std::vector<std::string> OS::getEnvironmentVariables() {
    std::vector<std::string> envVars;
    
    #ifdef _WIN32
        // On Windows, we use _environ
        extern char** environ;
        for (int i = 0; environ[i] != nullptr; ++i) {
            envVars.push_back(environ[i]);
        }
    #else
        // On Unix-like systems, we use the environ variable
        extern char** environ;
        for (int i = 0; environ[i] != nullptr; ++i) {
            envVars.push_back(environ[i]);
        }
    #endif
    
    return envVars;
}
