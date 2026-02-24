#include <omniscript/backends/llvm/IRGenerator.h>
#include <omniscript/backends/llvm/LLVMExternalFunctionResolver.h>
#include <omniscript/backends/llvm/ExternalFunctionResolvers/CLLVMResolver.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace Omniscript {
llvm::Function* CStdLibResolver::resolve(IRGenerator& generator, const std::string& name,
                                        llvm::FunctionType* funcType, LinkDependencies& deps) {
    
    // Define libraries to check in order of priority
    static const std::vector<std::pair<std::string, std::string>> librariesToCheck = {
#ifdef _WIN32
        {"msvcrt.dll", ""},           // Main C runtime (no link flag needed)
        {"ucrtbase.dll", ""},         // Universal C Runtime
        {"kernel32.dll", "kernel32"}, // Windows API
        {"user32.dll", "user32"},     // Windows User API
#else
        {"libc.so.6", ""},            // Main C library (no link flag needed)
        {"/lib/x86_64-linux-gnu/libc.so.6", ""},
        {"/lib64/libc.so.6", ""},
        {"libm.so.6", "m"},           // Math library
        {"/lib/x86_64-linux-gnu/libm.so.6", "m"},
        {"libpthread.so.0", "pthread"}, // Threading library
        {"/lib/x86_64-linux-gnu/libpthread.so.0", "pthread"},
        {"libdl.so.2", "dl"},         // Dynamic loading
        {"/lib/x86_64-linux-gnu/libdl.so.2", "dl"},
#ifdef __APPLE__
        {"/usr/lib/libSystem.dylib", ""},
        {"/usr/lib/libc.dylib", ""},
        {"/usr/lib/libm.dylib", "m"},
#endif
#endif
    };
    
    std::string foundLibrary = "";
    bool symbolFound = false;
    
    // Check each library for the symbol
    for (const auto& [libPath, linkName] : librariesToCheck) {
        if (symbolExistsInLibrary(libPath, name)) {
            foundLibrary = linkName;
            symbolFound = true;
            break;
        }
    }
    
    // If not found in any library, return nullptr
    if (!symbolFound) {
        return nullptr;
    }
    
    // Create the function
    llvm::Function* func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, name, generator.getCurrentModule()
    );
    
    // Apply platform-specific attributes
    applyPlatformSpecificAttributes(func, name);
    
    // Add library dependency if needed
    if (!foundLibrary.empty()) {
        LinkDependencies::LibraryInfo info;
        info.name = foundLibrary;
        info.isSystemLib = true;
        deps.addRequiredLibrary(foundLibrary, info);
    }
    
    return func;
}

// Helper function to check if symbol exists in a library
bool CStdLibResolver::symbolExistsInLibrary(const std::string& libPath, const std::string& symbolName) {
#ifdef _WIN32
    HMODULE hModule = LoadLibraryA(libPath.c_str());
    if (!hModule) {
        return false;
    }
    
    FARPROC proc = GetProcAddress(hModule, symbolName.c_str());
    FreeLibrary(hModule);
    
    return proc != nullptr;
    
#else
    void* handle = dlopen(libPath.c_str(), RTLD_LAZY | RTLD_NOLOAD);
    if (!handle) {
        handle = dlopen(libPath.c_str(), RTLD_LAZY);
    }
    if (!handle) {
        return false;
    }
    
    void* symbol = dlsym(handle, symbolName.c_str());
    dlclose(handle);
    
    return symbol != nullptr;
#endif
}

bool CStdLibResolver::isCStdLibFunction(const std::string& name) {
    // Windows
#ifdef _WIN32
    std::vector<std::string> windowsLibs = {
        "msvcrt.dll",      // Main C runtime
        "ucrtbase.dll",    // Universal C Runtime (Windows 10+)
        "api-ms-win-crt-stdio-l1-1-0.dll",
        "api-ms-win-crt-string-l1-1-0.dll",
        "api-ms-win-crt-math-l1-1-0.dll",
        "api-ms-win-crt-heap-l1-1-0.dll"
    };
    
    for (const auto& lib : windowsLibs) {
        HMODULE hModule = LoadLibraryA(lib.c_str());
        if (hModule) {
            FARPROC proc = GetProcAddress(hModule, name.c_str());
            FreeLibrary(hModule);
            if (proc != nullptr) {
                return true;
            }
        }
    }

// Linux
#elif __linux__
    std::vector<std::string> linuxLibs = {
        "libc.so.6",       // Main C library
        "/lib/x86_64-linux-gnu/libc.so.6",
        "/lib64/libc.so.6",
        "/usr/lib/libc.so.6"
    };
    
    for (const auto& lib : linuxLibs) {
        void* handle = dlopen(lib.c_str(), RTLD_LAZY | RTLD_NOLOAD);
        if (!handle) {
            handle = dlopen(lib.c_str(), RTLD_LAZY);
        }
        if (handle) {
            void* symbol = dlsym(handle, name.c_str());
            dlclose(handle);
            if (symbol != nullptr) {
                return true;
            }
        }
    }

// macOS
#elif __APPLE__
    std::vector<std::string> macLibs = {
        "/usr/lib/libSystem.dylib",     // Main system library
        "/usr/lib/libc.dylib",
        "libSystem.B.dylib"
    };
    
    for (const auto& lib : macLibs) {
        void* handle = dlopen(lib.c_str(), RTLD_LAZY | RTLD_NOLOAD);
        if (!handle) {
            handle = dlopen(lib.c_str(), RTLD_LAZY);
        }
        if (handle) {
            void* symbol = dlsym(handle, name.c_str());
            dlclose(handle);
            if (symbol != nullptr) {
                return true;
            }
        }
    }

// Other Unix-like systems
#else
    void* handle = dlopen(nullptr, RTLD_LAZY); // Check current process
    if (handle) {
        void* symbol = dlsym(handle, name.c_str());
        dlclose(handle);
        if (symbol != nullptr) {
            return true;
        }
    }
#endif

    return false;
}

void CStdLibResolver::applyPlatformSpecificAttributes(llvm::Function* func, const std::string& name) {
    // Set calling convention
    func->setCallingConv(getPlatformCallingConv(name));
    
    // Add common attributes for certain functions
    if (name == "malloc" || name == "calloc" || name == "realloc") {
        // func->addFnAttr(llvm::Attribute::NoAlias);
    }
    
    if (name == "strlen" || name == "strcmp" || name == "memcmp") {
        // func->addFnAttr(llvm::Attribute::ReadOnly);
        func->addFnAttr(llvm::Attribute::NoUnwind);
    }
    
    if (name == "memcpy" || name == "memset" || name == "strcpy") {
        func->addFnAttr(llvm::Attribute::NoUnwind);
    }
}

llvm::CallingConv::ID CStdLibResolver::getPlatformCallingConv(const std::string& name) {
    PlatformInfo::Platform platform = PlatformInfo::getCurrentPlatform();
    
    switch (platform) {
        case PlatformInfo::Windows:
            return llvm::CallingConv::C; // Use default C calling convention
        case PlatformInfo::WebAssembly:
            return llvm::CallingConv::C;
        default:
            return llvm::CallingConv::C;
    }
}

} // namespace Omniscript
