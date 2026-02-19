#pragma once
#include <omniscript/omniscript_pch.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/DynamicLibrary.h>

namespace Omniscript {
class IRGenerator;

class LinkDependencies {
public:
    struct LibraryInfo {
        std::string name;
        std::string path;           // Optional explicit path
        std::vector<std::string> linkerFlags;
        bool isSystemLib = false;
        bool isRequired = false;
    };

private:
    std::unordered_set<std::string> requiredLibraries_;
    std::unordered_map<std::string, LibraryInfo> libraryInfo_;
    std::unordered_set<std::string> librarySearchPaths_;

public:
    // Called by resolvers when they successfully resolve a function
    void addRequiredLibrary(const std::string& libName, const LibraryInfo& info);
    
    // Add library search path (for -L flags)
    void addLibrarySearchPath(const std::string& path);
    
    // Get only the libraries that are actually used
    std::vector<std::string> getLinkerFlags() const;
    
    bool hasLibrary(const std::string& libName) const;
    
    void clear();
    
    // Get all required library names
    std::vector<std::string> getRequiredLibraries() const;
    
    // Get library info
    const LibraryInfo* getLibraryInfo(const std::string& libName) const;
};

// 2. RESOLVER INTERFACE
class ExternalFunctionResolver {
public:
    virtual llvm::Function* resolve(IRGenerator& generator, const std::string& name, 
                                  llvm::FunctionType* funcType, LinkDependencies& deps) = 0;
    virtual ~ExternalFunctionResolver() = default;
    static bool isSystemLibrary(const std::string& libPath) ;

};

// Platform detection utility
class PlatformInfo {
public:
    enum Platform {
        Windows,
        Linux,
        MacOS,
        Android,
        iOS,
        FreeBSD,
        WebAssembly,
        Unknown
    };
    
    enum Architecture {
        x86,
        x86_64,
        ARM,
        ARM64,
        MIPS,
        RISC_V,
        WebAsm,
        UnknownArch
    };
    
    static Platform getCurrentPlatform();
    static Architecture getCurrentArchitecture();
    static std::string getPlatformString();
    static std::string getArchString();
    static bool isUnixLike();
    static bool isApple();
};

} // namespace Omniscript

// Usage example for extending to new platforms:
// class CustomGameConsoleResolver : public ExternalFunctionResolver {
// public:
//     llvm::Function* resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) override {
//         // Custom game console API resolution
//         if (isGameConsoleAPI(name)) {
//             auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, generator.getCurrentModule());
//             // Apply console-specific attributes
//             applyConsoleAttributes(func, name);
//             return func;
//         }
//         return nullptr;
//     }
    
// private:
//     bool isGameConsoleAPI(const std::string& name) {
//         // Implementation specific to your game console
//         return name.starts_with("console_") || name.starts_with("gpu_");
//     }
    
//     void applyConsoleAttributes(llvm::Function* func, const std::string& name) {
//         // Console-specific function attributes
//     }
// };

// Easy integration example:
/*
// In your IRGenerator setup:
auto smartResolver = std::make_unique<SmartPlatformResolver>();

// Add custom resolver for game console APIs
smartResolver->registerResolver("console_", std::make_unique<CustomGameConsoleResolver>());

// Add custom resolver for graphics APIs
smartResolver->registerResolver("gl", std::make_unique<OpenGLResolver>());
smartResolver->registerResolver("vk", std::make_unique<VulkanResolver>());

// Use the smart resolver
this->externalResolver = std::move(smartResolver);
*/