#pragma once
#include <omniscript/omniscript_pch.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/DynamicLibrary.h>

class IRGenerator;

// Base resolver interface
class ExternalFunctionResolver {
public:
    virtual llvm::Function* resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) = 0;
    virtual ~ExternalFunctionResolver() = default;
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

// Universal C Standard Library Resolver
class CStdLibResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) override;
    
private:
    void applyPlatformSpecificAttributes(llvm::Function* func, const std::string& name);
    llvm::CallingConv::ID getPlatformCallingConv(const std::string& name);
};

// Windows-specific resolvers
class WindowsAPIResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) override;
    
private:
    bool isKernel32Function(const std::string& name);
    bool isUser32Function(const std::string& name);
    bool isGdi32Function(const std::string& name);
    std::string getRequiredDLL(const std::string& name);
};

// POSIX/Unix resolver (Linux, macOS, BSD)
class PosixResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) override;
    
private:
    bool isPthreadFunction(const std::string& name);
    bool isSocketFunction(const std::string& name);
    bool isMathFunction(const std::string& name);
    std::vector<std::string> getRequiredLibraries(const std::string& name);
};

// Linux-specific resolver
class LinuxResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) override;
    
private:
    bool isGlibcFunction(const std::string& name);
    bool isSystemCallWrapper(const std::string& name);
};

// macOS/iOS resolver
class DarwinResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) override;
    
private:
    bool isFoundationFunction(const std::string& name);
    bool isCoreFoundationFunction(const std::string& name);
    bool isCocoaFunction(const std::string& name);
    std::string getRequiredFramework(const std::string& name);
};

// Android resolver
class AndroidResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) override;
    
private:
    bool isAndroidFunction(const std::string& name);
    bool isBionicFunction(const std::string& name);
    bool isJNIFunction(const std::string& name);
    std::string getRequiredLibrary(const std::string& name);
};

// WebAssembly resolver
class WebAssemblyResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) override;
    
private:
    bool isWASIFunction(const std::string& name);
    bool isEmscriptenFunction(const std::string& name);
    bool isWebAPIFunction(const std::string& name);
    void applyWasmAttributes(llvm::Function* func, const std::string& name);
};

// Generic dynamic library resolver (cross-platform)
class DynamicLibraryResolver : public ExternalFunctionResolver {
public:
    llvm::sys::DynamicLibrary dynLib;
    DynamicLibraryResolver(const std::string& libPath);
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) override;
    
private:
    std::string libPath_;
    static std::string normalizePath(const std::string& path);
};

// Generic static library resolver
class StaticLibraryResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) override;
};

// Smart resolver that automatically selects the best resolver for the platform
class SmartPlatformResolver : public ExternalFunctionResolver {
public:
    SmartPlatformResolver();
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) override;
    
    // Allow manual registration of custom resolvers
    void registerResolver(const std::string& pattern, std::unique_ptr<ExternalFunctionResolver> resolver);
    
private:
    std::vector<std::unique_ptr<ExternalFunctionResolver>> platformResolvers_;
    std::unordered_map<std::string, std::unique_ptr<ExternalFunctionResolver>> customResolvers_;
    
    void initializePlatformResolvers();
    ExternalFunctionResolver* findBestResolver(const std::string& name);
};

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