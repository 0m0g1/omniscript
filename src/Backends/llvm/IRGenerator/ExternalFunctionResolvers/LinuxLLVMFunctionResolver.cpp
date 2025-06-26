#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/LinuxLLVMResolver.h>

llvm::Function* LinuxResolver::resolve(IRGenerator& generator, const std::string& name, 
                                     llvm::FunctionType* funcType, LinkDependencies& deps) {
    if (PlatformInfo::getCurrentPlatform() != PlatformInfo::Linux) {
        return nullptr; // Only resolve on Linux
    }
    
    // Check if it's a known Linux-specific function
    if (!isGlibcFunction(name) && !isSystemCallWrapper(name)) {
        return nullptr;
    }
    
    // Create the function
    llvm::Function* func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, name, generator.getCurrentModule()
    );
    
    // Most Linux functions are in libc (no explicit linking needed)
    // Add specific library dependencies as needed
    
    return func;
}

bool LinuxResolver::isGlibcFunction(const std::string& name) {
    static const std::unordered_set<std::string> glibcFunctions = {
        "gnu_get_libc_version", "gnu_get_libc_release", "__libc_start_main",
        "backtrace", "backtrace_symbols", "backtrace_symbols_fd"
    };
    return glibcFunctions.find(name) != glibcFunctions.end();
}

bool LinuxResolver::isSystemCallWrapper(const std::string& name) {
    static const std::unordered_set<std::string> syscallWrappers = {
        "syscall", "clone", "pivot_root", "mount", "umount", "umount2",
        "sysinfo", "uname", "prctl", "capget", "capset", "personality"
    };
    return syscallWrappers.find(name) != syscallWrappers.end();
}
