#include <omniscript/engine/Backends/LLVM/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolver.h>

bool DarwinResolver::isFoundationFunction(const std::string& name) {
    return name.find("NS") == 0 || name.find("CF") == 0;
}

bool DarwinResolver::isCoreFoundationFunction(const std::string& name) {
    return name.find("CF") == 0;
}

bool DarwinResolver::isCocoaFunction(const std::string& name) {
    return name.find("NS") == 0;
}

std::string DarwinResolver::getRequiredFramework(const std::string& name) {
    if (isFoundationFunction(name)) return "Foundation";
    if (isCoreFoundationFunction(name)) return "CoreFoundation";
    if (isCocoaFunction(name)) return "Cocoa";
    return "";
}