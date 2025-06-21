#include <omniscript/engine/Backends/LLVM/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolver.h>

// Implementation helpers

// Platform detection implementation
inline PlatformInfo::Platform PlatformInfo::getCurrentPlatform() {
#ifdef _WIN32
    return Windows;
#elif defined(__ANDROID__)
    return Android;
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        return iOS;
    #else
        return MacOS;
    #endif
#elif defined(__linux__)
    return Linux;
#elif defined(__FreeBSD__)
    return FreeBSD;
#elif defined(__EMSCRIPTEN__)
    return WebAssembly;
#else
    return Unknown;
#endif
}

inline PlatformInfo::Architecture PlatformInfo::getCurrentArchitecture() {
#if defined(_M_X64) || defined(__x86_64__)
    return x86_64;
#elif defined(_M_IX86) || defined(__i386__)
    return x86;
#elif defined(_M_ARM64) || defined(__aarch64__)
    return ARM64;
#elif defined(_M_ARM) || defined(__arm__)
    return ARM;
#elif defined(__mips__)
    return MIPS;
#elif defined(__riscv)
    return RISC_V;
#elif defined(__EMSCRIPTEN__)
    return WebAsm;
#else
    return UnknownArch;
#endif
}

inline bool PlatformInfo::isUnixLike() {
    auto platform = getCurrentPlatform();
    return platform == Linux || platform == MacOS || platform == FreeBSD || platform == Android;
}

inline bool PlatformInfo::isApple() {
    auto platform = getCurrentPlatform();
    return platform == MacOS || platform == iOS;
}

inline std::string PlatformInfo::getPlatformString() {
    switch (getCurrentPlatform()) {
        case Windows: return "windows";
        case Linux: return "linux";
        case MacOS: return "macos";
        case Android: return "android";
        case iOS: return "ios";
        case FreeBSD: return "freebsd";
        case WebAssembly: return "wasm";
        default: return "unknown";
    }
}

// Smart resolver implementation
inline SmartPlatformResolver::SmartPlatformResolver() {
    initializePlatformResolvers();
}

inline void SmartPlatformResolver::initializePlatformResolvers() {
    // Always add C standard library resolver first
    platformResolvers_.push_back(std::make_unique<CStdLibResolver>());
    
    // Add platform-specific resolvers
    switch (PlatformInfo::getCurrentPlatform()) {
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
            
        case PlatformInfo::WebAssembly:
            platformResolvers_.push_back(std::make_unique<WebAssemblyResolver>());
            break;
            
        case PlatformInfo::FreeBSD:
            platformResolvers_.push_back(std::make_unique<PosixResolver>());
            break;
            
        default:
            // Fallback to POSIX for unknown Unix-like systems
            if (PlatformInfo::isUnixLike()) {
                platformResolvers_.push_back(std::make_unique<PosixResolver>());
            }
            break;
    }
}

inline llvm::Function* SmartPlatformResolver::resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) {
    // Try custom resolvers first
    for (const auto& [pattern, resolver] : customResolvers_) {
        if (name.find(pattern) != std::string::npos) {
            if (auto func = resolver->resolve(generator, name, funcType)) {
                return func;
            }
        }
    }
    
    // Try platform resolvers
    for (const auto& resolver : platformResolvers_) {
        if (auto func = resolver->resolve(generator, name, funcType)) {
            return func;
        }
    }
    
    return nullptr;
}

inline void SmartPlatformResolver::registerResolver(const std::string& pattern, std::unique_ptr<ExternalFunctionResolver> resolver) {
    customResolvers_[pattern] = std::move(resolver);
}