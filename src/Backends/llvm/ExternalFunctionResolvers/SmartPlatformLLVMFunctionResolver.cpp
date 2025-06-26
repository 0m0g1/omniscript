#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/CLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/LinuxLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/PosixLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/DarwinLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/AndroidLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/WindowsAPILLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/WebAssemblyLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/SmartPlatformLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/StaticLibraryLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/DynamicLibraryLLVMResolver.h>

SmartPlatformResolver::SmartPlatformResolver() {
    initializePlatformResolvers();
}

llvm::Function* SmartPlatformResolver::resolve(IRGenerator& generator, const std::string& name, 
                                             llvm::FunctionType* funcType, LinkDependencies& deps) {
    // First check custom resolvers
    for (const auto& [pattern, resolver] : customResolvers_) {
        if (name.find(pattern) != std::string::npos) {
            llvm::Function* result = resolver->resolve(generator, name, funcType, deps);
            if (result) {
                return result;
            }
        }
    }
    
    // Then try platform-specific resolvers
    for (auto& resolver : platformResolvers_) {
        llvm::Function* result = resolver->resolve(generator, name, funcType, deps);
        if (result) {
            return result;
        }
    }
    
    return nullptr; // Could not resolve
}

void SmartPlatformResolver::registerResolver(const std::string& pattern, 
                                           std::unique_ptr<ExternalFunctionResolver> resolver) {
    customResolvers_[pattern] = std::move(resolver);
}

void SmartPlatformResolver::initializePlatformResolvers() {
    // Always add C standard library resolver first
    platformResolvers_.push_back(std::make_unique<CStdLibResolver>());
    
    // Add platform-specific resolvers based on current platform
    PlatformInfo::Platform platform = PlatformInfo::getCurrentPlatform();
    
    switch (platform) {
        case PlatformInfo::Windows:
            platformResolvers_.push_back(std::make_unique<WindowsAPIResolver>());
            break;
            
        case PlatformInfo::Linux:
            platformResolvers_.push_back(std::make_unique<PosixResolver>());
            platformResolvers_.push_back(std::make_unique<LinuxResolver>());
            break;
            
        case PlatformInfo::MacOS:
        case PlatformInfo::iOS:
            platformResolvers_.push_back(std::make_unique<PosixResolver>());
            platformResolvers_.push_back(std::make_unique<DarwinResolver>());
            break;
            
        case PlatformInfo::Android:
            platformResolvers_.push_back(std::make_unique<PosixResolver>());
            platformResolvers_.push_back(std::make_unique<AndroidResolver>());
            break;
            
        case PlatformInfo::FreeBSD:
            platformResolvers_.push_back(std::make_unique<PosixResolver>());
            break;
            
        case PlatformInfo::WebAssembly:
            platformResolvers_.push_back(std::make_unique<WebAssemblyResolver>());
            break;
            
        default:
            // For unknown platforms, just use POSIX as fallback
            platformResolvers_.push_back(std::make_unique<PosixResolver>());
            break;
    }
}

ExternalFunctionResolver* SmartPlatformResolver::findBestResolver(const std::string& name) {
    // This method could implement more sophisticated logic to choose
    // the best resolver based on function name patterns, frequency of use, etc.
    
    // For now, just return the first resolver that can handle the function
    for (auto& resolver : platformResolvers_) {
        // We can't easily check if a resolver can handle a function without
        // actually trying to resolve it, so this method might not be as useful
        // as initially thought. The resolve() method already iterates through resolvers.
    }
    
    return nullptr;
}