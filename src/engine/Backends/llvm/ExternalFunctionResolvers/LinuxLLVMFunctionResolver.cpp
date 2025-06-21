#include <omniscript/engine/Backends/LLVM/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/LLVMExternalFunctionResolver.h>

// LinuxResolver Implementation
llvm::Function* LinuxResolver::resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) {
    if (PlatformInfo::getCurrentPlatform() != PlatformInfo::Linux) {
        return nullptr;
    }
    
    if (isGlibcFunction(name) || isSystemCallWrapper(name)) {
        auto func = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            name,
            generator.getCurrentModule()
        );
        
        func->setCallingConv(llvm::CallingConv::C);
        return func;
    }
    
    return nullptr;
}

bool LinuxResolver::isGlibcFunction(const std::string& name) {
    static const std::unordered_set<std::string> glibcFunctions = {
        "backtrace", "backtrace_symbols", "dlopen", "dlclose", "dlsym", "dlerror",
        "getpid", "getppid", "getuid", "getgid", "geteuid", "getegid",
        "setuid", "setgid", "seteuid", "setegid", "fork", "execve", "waitpid"
    };
    
    return glibcFunctions.find(name) != glibcFunctions.end();
}

bool LinuxResolver::isSystemCallWrapper(const std::string& name) {
    static const std::unordered_set<std::string> syscallWrappers = {
        "syscall", "open", "read", "write", "lseek", "stat", "fstat",
        "mmap", "munmap", "mprotect", "brk", "sbrk", "ioctl", "fcntl"
    };
    
    return syscallWrappers.find(name) != syscallWrappers.end();
}

// DarwinResolver Implementation
llvm::Function* DarwinResolver::resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) {
    if (!PlatformInfo::isApple()) {
        return nullptr;
    }
    
    if (isFoundationFunction(name) || isCoreFoundationFunction(name) || isCocoaFunction(name)) {
        auto func = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            name,
            generator.getCurrentModule()
        );
        
        func->setCallingConv(llvm::CallingConv::C);
        return func;
    }
    
    return nullptr;
}