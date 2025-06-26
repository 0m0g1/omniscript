#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/DynamicLibraryLLVMResolver.h>

DynamicLibraryResolver::DynamicLibraryResolver(const std::string& libPath) 
    : libPath_(normalizePath(libPath)) {
    std::string error;
    dynLib = llvm::sys::DynamicLibrary::getPermanentLibrary(libPath_.c_str(), &error);
    if (!dynLib.isValid()) {
        llvm::errs() << "Failed to load dynamic library " << libPath_ << ": " << error << "\n";
    }
}

llvm::Function* DynamicLibraryResolver::resolve(IRGenerator& generator, const std::string& name, 
                                              llvm::FunctionType* funcType, LinkDependencies& deps) {
    if (!dynLib.isValid()) {
        return nullptr;
    }
    
    // Check if the symbol exists in the dynamic library
    void* symbol = dynLib.getAddressOfSymbol(name.c_str());
    if (!symbol) {
        return nullptr;
    }
    
    // Create the function
    llvm::Function* func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, name, generator.getCurrentModule()
    );
    
    // Add library dependency
    LinkDependencies::LibraryInfo info;
    info.name = libPath_;
    info.path = libPath_;
    deps.addRequiredLibrary(libPath_, info);
    
    return func;
}

std::string DynamicLibraryResolver::normalizePath(const std::string& path) {
    // Convert relative paths to absolute, handle platform-specific extensions
    std::string normalized = path;
    
    // Add platform-specific extension if not present
    PlatformInfo::Platform platform = PlatformInfo::getCurrentPlatform();
    if (platform == PlatformInfo::Windows && normalized.find(".dll") == std::string::npos) {
        normalized += ".dll";
    } else if (platform == PlatformInfo::MacOS && normalized.find(".dylib") == std::string::npos && 
               normalized.find(".so") == std::string::npos) {
        normalized += ".dylib";
    } else if (PlatformInfo::isUnixLike() && platform != PlatformInfo::MacOS && 
               normalized.find(".so") == std::string::npos) {
        normalized += ".so";
    }
    
    return normalized;
}