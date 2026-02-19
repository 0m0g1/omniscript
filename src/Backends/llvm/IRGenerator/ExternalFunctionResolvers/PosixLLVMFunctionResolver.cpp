#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/PosixLLVMResolver.h>

namespace Omniscript {
llvm::Function* PosixResolver::resolve(IRGenerator& generator, const std::string& name, 
                                     llvm::FunctionType* funcType, LinkDependencies& deps) {
    if (!PlatformInfo::isUnixLike()) {
        return nullptr; // Only resolve on Unix-like systems
    }
    
    std::vector<std::string> requiredLibs = getRequiredLibraries(name);
    if (requiredLibs.empty()) {
        return nullptr; // Unknown POSIX function
    }
    
    // Create the function
    llvm::Function* func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, name, generator.getCurrentModule()
    );
    
    // Add library dependencies
    for (const auto& lib : requiredLibs) {
        if (!lib.empty()) {
            LinkDependencies::LibraryInfo info;
            info.name = lib;
            info.isSystemLib = true;
            deps.addRequiredLibrary(lib, info);
        }
    }
    
    return func;
}

bool PosixResolver::isPthreadFunction(const std::string& name) {
    return name.find("pthread_") == 0;
}

bool PosixResolver::isSocketFunction(const std::string& name) {
    static const std::unordered_set<std::string> socketFunctions = {
        "socket", "bind", "listen", "accept", "connect", "send", "recv",
        "sendto", "recvfrom", "shutdown", "close", "getsockopt", "setsockopt"
    };
    return socketFunctions.find(name) != socketFunctions.end();
}

bool PosixResolver::isMathFunction(const std::string& name) {
    static const std::unordered_set<std::string> mathFunctions = {
        "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
        "sinh", "cosh", "tanh", "exp", "log", "log10", "pow",
        "sqrt", "ceil", "floor", "fabs", "ldexp", "frexp"
    };
    return mathFunctions.find(name) != mathFunctions.end();
}

std::vector<std::string> PosixResolver::getRequiredLibraries(const std::string& name) {
    if (isPthreadFunction(name)) return {"pthread"};
    if (isMathFunction(name)) return {"m"};
    if (isSocketFunction(name)) return {""}; // Usually in libc
    
    // Add more POSIX function mappings here
    static const std::unordered_map<std::string, std::vector<std::string>> functionToLibs = {
        {"dlopen", {"dl"}}, {"dlsym", {"dl"}}, {"dlclose", {"dl"}}, {"dlerror", {"dl"}},
        {"crypt", {"crypt"}}, {"gethostbyname", {"nsl", "resolv"}}, {"inet_addr", {"nsl"}}
    };
    
    auto it = functionToLibs.find(name);
    if (it != functionToLibs.end()) {
        return it->second;
    }
    
    return {}; // Unknown function
}

} // namespace Omniscript
