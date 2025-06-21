#include <omniscript/engine/Backends/LLVM/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolver.h>

// DynamicLibraryResolver Implementation
DynamicLibraryResolver::DynamicLibraryResolver(const std::string& libPath) : libPath_(libPath) {
    std::string errMsg;
    std::string normalizedPath = normalizePath(libPath);
    dynLib = llvm::sys::DynamicLibrary::getPermanentLibrary(normalizedPath.c_str(), &errMsg);
    if (!dynLib.isValid()) {
        throw std::runtime_error("Failed to load library: " + errMsg);
    }
}

llvm::Function* DynamicLibraryResolver::resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) {
    void* symbol = dynLib.getAddressOfSymbol(name.c_str());
    if (!symbol) return nullptr;
    
    return llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        name,
        generator.getCurrentModule()
    );
}

std::string DynamicLibraryResolver::normalizePath(const std::string& path) {
    // Platform-specific path normalization
    std::string normalized = path;
    
    switch (PlatformInfo::getCurrentPlatform()) {
        case PlatformInfo::Windows:
            // Ensure .dll extension
            if (normalized.find(".dll") == std::string::npos) {
                normalized += ".dll";
            }
            break;
            
        case PlatformInfo::MacOS:
        case PlatformInfo::iOS:
            // Ensure .dylib extension
            if (normalized.find(".dylib") == std::string::npos && 
                normalized.find(".framework") == std::string::npos) {
                normalized += ".dylib";
            }
            break;
            
        default:
            // Unix-like systems use .so
            if (normalized.find(".so") == std::string::npos) {
                normalized += ".so";
            }
            break;
    }
    
    return normalized;
}