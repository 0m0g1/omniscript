# TargetInfo

## Purpose
The `TargetInfo` component in the OmniScript++ (OS) compiler provides utilities for detecting and managing target architectures, operating systems, and target triples for cross-compilation and code generation. It enables the compiler to adapt to different platforms (e.g., x86_64, ARM64, Windows, Linux) by offering architecture and OS metadata, triple generation, and compatibility checks. This component is crucial for professional-grade compilers, particularly those using LLVM for code generation, as it ensures correct configuration of target-specific parameters, such as pointer sizes, library paths, and system features, facilitating portable and efficient code output.

## Declarations
Below is the header file for `TargetInfo`, with `<omniscript/omniscript_pch.h>` replaced by the necessary standard library includes.

```cpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <sstream>
#include <cstdio>

// Auto-detect target architecture and OS
#if !defined(TARGET_32BIT) && !defined(TARGET_64BIT)
    #if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(_M_ARM64)
        #define TARGET_64BIT 1
    #elif defined(__i386__) || defined(_M_IX86) || defined(__arm__) || defined(_M_ARM)
        #define TARGET_32BIT 1
    #else
        #error "Unknown architecture: define TARGET_32BIT or TARGET_64BIT manually"
    #endif
#endif

// Forward declarations for Config enums
enum class TargetArch {
    Auto,        // Detect host architecture
    X86_64,
    ARM64,
    X86_32,
    ARM32,
    RISCV64,
    WASM32,
    WASM64
};

enum class TargetOS {
    Auto,        // Detect host OS
    Linux,
    Windows,
    MacOS,
    FreeBSD,
    Android,
    iOS,
    WebAssembly
};

namespace TargetInfo {

    // Architecture detection and utilities
    struct ArchitectureInfo {
        std::string name;
        std::string llvmName;
        std::string gccName;
        std::string msvcName;
        bool is64Bit;
        bool isLittleEndian;
        int pointerSize;
        int alignment;
        std::vector<std::string> features;
        std::vector<std::string> aliases;
    };

    // Operating system detection and utilities
    struct OSInfo {
        std::string name;
        std::string llvmName;
        std::string fileExtension;
        std::string sharedLibExtension;
        std::string staticLibExtension;
        std::string objectFileExtension;
        std::string executableExtension;
        bool isUnixLike;
        bool supportsPIC;
        std::vector<std::string> systemLibraries;
    };

    // Target triple utilities
    struct TargetTriple {
        std::string arch;
        std::string vendor;
        std::string os;
        std::string environment;
        
        std::string toString() const {
            std::string result = arch;
            if (!vendor.empty()) result += "-" + vendor;
            if (!os.empty()) result += "-" + os;
            if (!environment.empty()) result += "-" + environment;
            return result;
        }
        
        static TargetTriple parse(const std::string& triple);
    };

    // Host detection functions
    TargetArch detectHostArchitecture();
    TargetOS detectHostOS();
    std::string detectHostTriple();
    
    // Architecture utilities
    ArchitectureInfo getArchitectureInfo(TargetArch arch);
    std::string getArchitectureName(TargetArch arch);
    std::string getLLVMArchName(TargetArch arch);
    bool isArchitecture64Bit(TargetArch arch);
    int getPointerSize(TargetArch arch);
    std::vector<std::string> getArchitectureFeatures(TargetArch arch);
    
    // OS utilities
    OSInfo getOSInfo(TargetOS os);
    std::string getOSName(TargetOS os);
    std::string getLLVMOSName(TargetOS os);
    std::string getExecutableExtension(TargetOS os);
    std::string getSharedLibExtension(TargetOS os);
    std::string getStaticLibExtension(TargetOS os);
    std::string getObjectFileExtension(TargetOS os);
    bool isUnixLikeOS(TargetOS os);
    
    // Triple generation and validation
    std::string generateTriple(TargetArch arch, TargetOS os, const std::string& vendor = "unknown");
    std::string generateTriple(TargetArch arch, TargetOS os, const std::string& vendor, const std::string& environment);
    bool isValidTriple(const std::string& triple);
    TargetTriple normalizeTriple(const std::string& triple);
    
    // Cross-compilation utilities
    bool isCrossCompilation(TargetArch targetArch, TargetOS targetOS);
    std::vector<std::string> getRequiredSystemLibraries(TargetOS os);
    std::vector<std::string> getDefaultLibraryPaths(TargetOS os, TargetArch arch);
    std::vector<std::string> getDefaultIncludePaths(TargetOS os, TargetArch arch);
    
    // Feature detection
    bool supportsFeature(TargetArch arch, const std::string& feature);
    std::vector<std::string> getAvailableFeatures(TargetArch arch);
    std::string getDefaultCPUForArch(TargetArch arch);
    
    // Compatibility checks
    bool isArchitectureCompatible(TargetArch source, TargetArch target);
    bool isOSCompatible(TargetOS source, TargetOS target);
    
    // Debug and utility functions
    void printHostInfo();
    void printTargetInfo(TargetArch arch, TargetOS os);
    std::string getTargetSummary(TargetArch arch, TargetOS os);

} // namespace TargetInfo
```

### Explanation
- **Macros**: `TARGET_32BIT` and `TARGET_64BIT` are defined based on compiler-defined architecture macros (e.g., `__x86_64__`, `__arm__`), ensuring automatic detection of the host architecture.
- **Enums**:
  - `TargetArch`: Lists supported architectures (e.g., `X86_64`, `ARM64`, `WASM32`) with an `Auto` option for host detection.
  - `TargetOS`: Lists supported operating systems (e.g., `Linux`, `Windows`, `WebAssembly`) with an `Auto` option.
- **Structures**:
  - `ArchitectureInfo`: Contains metadata like architecture name, LLVM/GCC/MSVC names, pointer size, endianness, alignment, and supported features.
  - `OSInfo`: Includes OS name, LLVM name, file extensions (e.g., `.so`, `.exe`), and system libraries.
  - `TargetTriple`: Represents a target triple (e.g., `x86_64-pc-linux-gnu`) with fields for architecture, vendor, OS, and environment, plus a `toString()` method and static `parse()` function.
- **Functions**:
  - **Host Detection**: `detectHostArchitecture()`, `detectHostOS()`, `detectHostTriple()` identify the host platform.
  - **Architecture Utilities**: Functions like `getArchitectureInfo()`, `getLLVMArchName()`, and `getPointerSize()` provide architecture-specific details.
  - **OS Utilities**: Functions like `getOSInfo()`, `getExecutableExtension()`, and `isUnixLikeOS()` provide OS-specific details.
  - **Triple Management**: `generateTriple()`, `isValidTriple()`, and `normalizeTriple()` handle LLVM-compatible target triples.
  - **Cross-Compilation**: `isCrossCompilation()`, `getRequiredSystemLibraries()`, and path functions (`getDefaultLibraryPaths()`, `getDefaultIncludePaths()`) support cross-compilation.
  - **Feature Detection**: `supportsFeature()`, `getAvailableFeatures()`, and `getDefaultCPUForArch()` manage architecture features.
  - **Compatibility**: `isArchitectureCompatible()` and `isOSCompatible()` check platform compatibility.
  - **Debug Utilities**: `printHostInfo()`, `printTargetInfo()`, and `getTargetSummary()` aid debugging.

## Definitions
Below is the implementation for `TargetInfo`, included inline in the header file as per the provided code. For clarity, the implementation is reproduced here with the same standard library includes.

```cpp
// Implementation
namespace TargetInfo {

    // Host detection implementations
    inline TargetArch detectHostArchitecture() {
        #if defined(__x86_64__) || defined(_M_X64)
            return TargetArch::X86_64;
        #elif defined(__aarch64__) || defined(_M_ARM64)
            return TargetArch::ARM64;
        #elif defined(__i386__) || defined(_M_IX86)
            return TargetArch::X86_32;
        #elif defined(__arm__) || defined(_M_ARM)
            return TargetArch::ARM32;
        #elif defined(__riscv) && (__riscv_xlen == 64)
            return TargetArch::RISCV64;
        #elif defined(__wasm32__)
            return TargetArch::WASM32;
        #elif defined(__wasm64__)
            return TargetArch::WASM64;
        #else
            return TargetArch::Auto;
        #endif
    }

    inline TargetOS detectHostOS() {
        #if defined(__linux__)
            return TargetOS::Linux;
        #elif defined(_WIN32) || defined(_WIN64)
            return TargetOS::Windows;
        #elif defined(__APPLE__)
            #include <TargetConditionals.h>
            #if TARGET_OS_IPHONE
                return TargetOS::iOS;
            #else
                return TargetOS::MacOS;
            #endif
        #elif defined(__FreeBSD__)
            return TargetOS::FreeBSD;
        #elif defined(__ANDROID__)
            return TargetOS::Android;
        #elif defined(__EMSCRIPTEN__)
            return TargetOS::WebAssembly;
        #else
            return TargetOS::Auto;
        #endif
    }

    inline std::string detectHostVendor(TargetOS os) {
        switch (os) {
            case TargetOS::Windows:
                #ifdef _MSC_VER
                    return "pc";  // Microsoft compiler typically uses "pc"
                #else
                    return "w64";  // MinGW uses w64
                #endif
            case TargetOS::MacOS:
            case TargetOS::iOS:
                return "apple";
            case TargetOS::Linux:
                return "pc";  // Most Linux distributions use "pc"
            case TargetOS::Android:
                return "linux";  // Android uses "linux" as vendor
            case TargetOS::FreeBSD:
                return "unknown";  // FreeBSD commonly uses "unknown"
            case TargetOS::WebAssembly:
                return "unknown";
            default:
                return "unknown";
        }
    }

    inline std::string detectHostEnvironment(TargetOS os, TargetArch arch) {
        switch (os) {
            case TargetOS::Windows:
                #ifdef _MSC_VER
                    return "msvc";
                #elif defined(__MINGW32__) || defined(__MINGW64__)
                    return "gnu";
                #else
                    return "msvc";  // Default to msvc on Windows
                #endif
            case TargetOS::Linux:
                return "gnu";
            case TargetOS::Android:
                return "android";
            case TargetOS::MacOS:
                return "macho";  // or could be empty
            case TargetOS::iOS:
                return "macho";
            case TargetOS::FreeBSD:
                return "";  // Usually empty
            case TargetOS::WebAssembly:
                return "";
            default:
                return "";
        }
    }

    inline std::string detectHostTriple() {
        auto arch = detectHostArchitecture();
        auto os = detectHostOS();
        std::string vendor = detectHostVendor(os);
        std::string environment = detectHostEnvironment(os, arch);
        
        return generateTriple(arch, os, vendor, environment);
    }

    // Architecture info lookup
    inline ArchitectureInfo getArchitectureInfo(TargetArch arch) {
        static const std::unordered_map<TargetArch, ArchitectureInfo> archMap = {
            {TargetArch::X86_64, {
                "x86_64", "x86_64", "x86_64", "x64", true, true, 8, 8,
                {"sse", "sse2", "sse3", "ssse3", "sse4.1", "sse4.2", "avx", "avx2", "bmi", "bmi2"},
                {"x86-64", "amd64", "x64"}
            }},
            {TargetArch::ARM64, {
                "arm64", "aarch64", "aarch64", "arm64", true, true, 8, 8,
                {"neon", "crypto", "crc", "lse", "fp16", "sve"},
                {"aarch64", "arm64"}
            }},
            {TargetArch::X86_32, {
                "x86", "i386", "i386", "x86", false, true, 4, 4,
                {"sse", "sse2", "mmx"},
                {"i386", "i486", "i586", "i686", "x86"}
            }},
            {TargetArch::ARM32, {
                "arm", "arm", "arm", "arm", false, true, 4, 4,
                {"neon", "vfp", "thumb", "thumb2"},
                {"armv7", "armv6", "arm"}
            }},
            {TargetArch::RISCV64, {
                "riscv64", "riscv64", "riscv64", "riscv64", true, true, 8, 8,
                {"rv64i", "rv64im", "rv64g", "rv64gc"},
                {"riscv64"}
            }},
            {TargetArch::WASM32, {
                "wasm32", "wasm32", "wasm32", "wasm32", false, true, 4, 4,
                {"simd128", "atomics", "bulk-memory", "sign-ext"},
                {"wasm"}
            }},
            {TargetArch::WASM64, {
                "wasm64", "wasm64", "wasm64", "wasm64", true, true, 8, 8,
                {"simd128", "atomics", "bulk-memory", "sign-ext"},
                {"wasm64"}
            }}
        };
        
        auto it = archMap.find(arch);
        if (it != archMap.end()) {
            return it->second;
        }
        
        // Return auto-detected for Auto
        if (arch == TargetArch::Auto) {
            return getArchitectureInfo(detectHostArchitecture());
        }
        
        // Fallback
        return {"unknown", "unknown", "unknown", "unknown", false, true, 4, 4, {}, {}};
    }

    // OS info lookup
    inline OSInfo getOSInfo(TargetOS os) {
        static const std::unordered_map<TargetOS, OSInfo> osMap = {
            {TargetOS::Linux, {
                "linux", "linux", "", ".so", ".a", ".o", "", true, true,
                {"c", "m", "pthread", "dl", "rt"}
            }},
            {TargetOS::Windows, {
                "windows", "windows", ".exe", ".dll", ".lib", ".obj", ".exe", false, false,
                {"kernel32", "user32", "gdi32", "msvcrt"}
            }},
            {TargetOS::MacOS, {
                "macos", "darwin", "", ".dylib", ".a", ".o", "", true, true,
                {"c", "m", "pthread", "System"}
            }},
            {TargetOS::FreeBSD, {
                "freebsd", "freebsd", "", ".so", ".a", ".o", "", true, true,
                {"c", "m", "pthread", "execinfo"}
            }},
            {TargetOS::Android, {
                "android", "android", "", ".so", ".a", ".o", "", true, true,
                {"c", "m", "log", "android"}
            }},
            {TargetOS::iOS, {
                "ios", "ios", "", ".dylib", ".a", ".o", "", true, true,
                {"c", "m", "pthread", "Foundation"}
            }},
            {TargetOS::WebAssembly, {
                "wasm", "wasm", ".wasm", ".wasm", ".a", ".o", ".wasm", false, false,
                {}
            }}
        };
        
        auto it = osMap.find(os);
        if (it != osMap.end()) {
            return it->second;
        }
        
        // Return auto-detected for Auto
        if (os == TargetOS::Auto) {
            return getOSInfo(detectHostOS());
        }
        
        // Fallback
        return {"unknown", "unknown", "", ".so", ".a", ".o", "", true, true, {}};
    }

    // Utility function implementations
    inline std::string getArchitectureName(TargetArch arch) {
        return getArchitectureInfo(arch).name;
    }

    inline std::string getLLVMArchName(TargetArch arch) {
        return getArchitectureInfo(arch).llvmName;
    }

    inline bool isArchitecture64Bit(TargetArch arch) {
        return getArchitectureInfo(arch).is64Bit;
    }

    inline int getPointerSize(TargetArch arch) {
        return getArchitectureInfo(arch).pointerSize;
    }

    inline std::vector<std::string> getArchitectureFeatures(TargetArch arch) {
        return getArchitectureInfo(arch).features;
    }

    inline std::string getOSName(TargetOS os) {
        return getOSInfo(os).name;
    }

    inline std::string getLLVMOSName(TargetOS os) {
        return getOSInfo(os).llvmName;
    }

    inline std::string getExecutableExtension(TargetOS os) {
        return getOSInfo(os).executableExtension;
    }

    inline std::string getSharedLibExtension(TargetOS os) {
        return getOSInfo(os).sharedLibExtension;
    }

    inline std::string getStaticLibExtension(TargetOS os) {
        return getOSInfo(os).staticLibExtension;
    }

    inline std::string getObjectFileExtension(TargetOS os) {
        return getOSInfo(os).objectFileExtension;
    }

    inline bool isUnixLikeOS(TargetOS os) {
        return getOSInfo(os).isUnixLike;
    }

    // Triple generation
    inline std::string generateTriple(TargetArch arch, TargetOS os, const std::string& vendor) {
        return generateTriple(arch, os, vendor, "");
    }

    inline std::string generateTriple(TargetArch arch, TargetOS os, const std::string& vendor, const std::string& environment) {
        TargetTriple triple;
        triple.arch = getLLVMArchName(arch);
        
        // Use smart vendor detection if empty
        if (vendor.empty()) {
            triple.vendor = detectHostVendor(os);
        } else {
            triple.vendor = vendor;
        }
        
        triple.os = getLLVMOSName(os);
        
        // Use smart environment detection if empty
        if (environment.empty()) {
            triple.environment = detectHostEnvironment(os, arch);
        } else {
            triple.environment = environment;
        }
        
        return triple.toString();
    }

    inline std::string normalizeArchName(const std::string& arch) {
        std::string lowerArch = arch;
        std::transform(lowerArch.begin(), lowerArch.end(), lowerArch.begin(), ::tolower);
        
        if (lowerArch == "x86-64" || lowerArch == "amd64" || lowerArch == "x64") {
            return "x86_64";
        } else if (lowerArch == "arm64") {
            return "aarch64";
        } else if (lowerArch == "i386" || lowerArch == "i486" || lowerArch == "i586" || lowerArch == "i686") {
            return "i386";
        } else if (lowerArch == "armv6" || lowerArch == "armv7") {
            return "arm";
        } else if (lowerArch == "wasm") {
            return "wasm32";
        }
        return lowerArch;
    }

    inline std::string normalizeVendorName(const std::string& vendor) {
        std::string lowerVendor = vendor;
        std::transform(lowerVendor.begin(), lowerVendor.end(), lowerVendor.begin(), ::tolower);
        
        if (lowerVendor == "microsoft" || lowerVendor == "ms") {
            return "pc";
        }
        return lowerVendor;
    }

    inline std::string normalizeOSName(const std::string& os) {
        std::string lowerOS = os;
        std::transform(lowerOS.begin(), lowerOS.end(), lowerOS.begin(), ::tolower);
        
        if (lowerOS == "macos" || lowerOS == "osx") {
            return "darwin";
        } else if (lowerOS == "win32" || lowerOS == "mingw32" || lowerOS == "cygwin") {
            return "windows";
        }
        return lowerOS;
    }

    inline std::string normalizeEnvironmentName(const std::string& env) {
        std::string lowerEnv = env;
        std::transform(lowerEnv.begin(), lowerEnv.end(), lowerEnv.begin(), ::tolower);
        return lowerEnv;
    }

    // Helper function to identify known OS names
    inline bool isKnownOS(const std::string& name) {
        static const std::unordered_set<std::string> knownOSs = {
            "linux", "windows", "win32", "darwin", "macos", "freebsd", 
            "android", "ios", "wasm", "emscripten", "none", "elf"
        };
        
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        return knownOSs.find(lowerName) != knownOSs.end();
    }

    // Enhanced triple parsing with better error handling
    inline TargetTriple TargetTriple::parse(const std::string& triple) {
        TargetTriple result;
        
        if (triple.empty()) {
            return result;
        }
        
        // Split by dashes, but be careful about edge cases
        std::vector<std::string> parts;
        std::stringstream ss(triple);
        std::string part;
        
        while (std::getline(ss, part, '-')) {
            if (!part.empty()) {
                parts.push_back(part);
            }
        }
        
        if (parts.empty()) {
            return result;
        }
        
        // More intelligent parsing based on known patterns
        if (parts.size() == 1) {
            // Just architecture
            result.arch = parts[0];
        } else if (parts.size() == 2) {
            // Could be arch-os or arch-vendor
            result.arch = parts[0];
            
            // Check if second part looks like an OS
            if (isKnownOS(parts[1])) {
                result.os = parts[1];
                result.vendor = "unknown";
            } else {
                result.vendor = parts[1];
            }
        } else if (parts.size() == 3) {
            result.arch = parts[0];
            result.vendor = parts[1];
            result.os = parts[2];
        } else if (parts.size() >= 4) {
            result.arch = parts[0];
            result.vendor = parts[1];
            result.os = parts[2];
            result.environment = parts[3];
            
            // Handle cases with more than 4 parts by joining the rest
            for (size_t i = 4; i < parts.size(); ++i) {
                result.environment += "-" + parts[i];
            }
        }
        
        return result;
    }

    inline std::string inferEnvironment(const std::string& os, const std::string& vendor) {
        if (os == "windows") {
            if (vendor == "pc") {
                return "msvc";
            } else if (vendor == "w64") {
                return "gnu";
            }
            return "msvc";  // Default for Windows
        } else if (os == "linux") {
            return "gnu";
        } else if (os == "android") {
            return "android";
        } else if (os == "darwin") {
            return "";  // macOS typically doesn't specify environment
        }
        return "";
    }

    inline bool isValidTriple(const std::string& triple) {
        if (triple.empty()) {
            return false;
        }
        
        TargetTriple parsed = TargetTriple::parse(triple);
        
        // Architecture must be present and valid
        if (parsed.arch.empty()) {
            return false;
        }
        
        static const std::unordered_set<std::string> validArchs = {
            "x86_64", "x86-64", "amd64", "x64",
            "aarch64", "arm64",
            "i386", "i486", "i586", "i686", "x86",
            "arm", "armv6", "armv7", "armv8",
            "riscv64", "riscv32",
            "wasm32", "wasm64", "wasm",
            "mips", "mips64", "mipsel", "mips64el",
            "powerpc", "powerpc64", "ppc", "ppc64",
            "sparc", "sparc64", "sparcv9"
        };
        
        std::string normalizedArch = normalizeArchName(parsed.arch);
        bool validArch = false;
        for (const auto& arch : validArchs) {
            if (normalizedArch == normalizeArchName(arch)) {
                validArch = true;
                break;
            }
        }
        
        if (!validArch) {
            return false;
        }
        
        // Validate OS if present
        if (!parsed.os.empty()) {
            static const std::unordered_set<std::string> validOSs = {
                "linux", "windows", "win32", "darwin", "macos", "freebsd", 
                "android", "ios", "wasm", "emscripten", "none", "elf",
                "netbsd", "openbsd", "solaris", "haiku"
            };
            
            std::string normalizedOS = normalizeOSName(parsed.os);
            if (validOSs.find(normalizedOS) == validOSs.end()) {
                return false;
            }
        }
        
        // Basic format validation
        size_t dashCount = std::count(triple.begin(), triple.end(), '-');
        if (dashCount > 4) {  // Allow up to 4 dashes for complex environments
            return false;
        }
        
        return true;
    }

    inline std::string getCanonicalTriple(TargetArch arch, TargetOS os) {
        std::string vendor = detectHostVendor(os);
        std::string environment = detectHostEnvironment(os, arch);
        return generateTriple(arch, os, vendor, environment);
    }

    inline std::string getCanonicalHostTriple() {
        auto arch = detectHostArchitecture();
        auto os = detectHostOS();
        return getCanonicalTriple(arch, os);
    }

    inline TargetTriple normalizeTriple(const std::string& triple) {
        TargetTriple parsed = TargetTriple::parse(triple);
        TargetTriple normalized;
        
        // Normalize architecture
        normalized.arch = normalizeArchName(parsed.arch);
        
        // Normalize vendor with smarter defaults
        if (parsed.vendor.empty()) {
            // Infer vendor from OS if not specified
            if (parsed.os == "darwin" || parsed.os == "macos" || parsed.os == "ios") {
                normalized.vendor = "apple";
            } else if (parsed.os == "windows" || parsed.os == "win32") {
                normalized.vendor = "pc";
            } else if (parsed.os == "linux") {
                normalized.vendor = "pc";
            } else {
                normalized.vendor = "unknown";
            }
        } else {
            normalized.vendor = normalizeVendorName(parsed.vendor);
        }
        
        // Normalize OS
        normalized.os = normalizeOSName(parsed.os);
        
        // Normalize environment with smarter defaults
        if (parsed.environment.empty()) {
            normalized.environment = inferEnvironment(normalized.os, normalized.vendor);
        } else {
            normalized.environment = normalizeEnvironmentName(parsed.environment);
        }
        
        return normalized;
    }

    // Cross-compilation utilities
    inline bool isCrossCompilation(TargetArch targetArch, TargetOS targetOS) {
        return targetArch != detectHostArchitecture() || targetOS != detectHostOS();
    }

    inline std::vector<std::string> getRequiredSystemLibraries(TargetOS os) {
        return getOSInfo(os).systemLibraries;
    }

    inline std::vector<std::string> getDefaultLibraryPaths(TargetOS os, TargetArch arch) {
        std::vector<std::string> paths;
        bool is64Bit = isArchitecture64Bit(arch);
        
        switch (os) {
            case TargetOS::Linux:
                paths = {"/usr/lib", "/usr/local/lib", "/lib"};
                if (is64Bit) {
                    paths.insert(paths.begin(), {"/usr/lib64", "/usr/local/lib64", "/lib64"});
                }
                break;
            case TargetOS::MacOS:
                paths = {"/usr/lib", "/usr/local/lib", "/System/Library/Frameworks"};
                break;
            case TargetOS::Windows:
                paths = {"C:/Windows/System32", "C:/Windows/SysWOW64"};
                break;
            case TargetOS::FreeBSD:
                paths = {"/usr/lib", "/usr/local/lib", "/lib"};
                break;
            default:
                break;
        }
        
        return paths;
    }

    inline std::vector<std::string> getDefaultIncludePaths(TargetOS os, TargetArch arch) {
        std::vector<std::string> paths;
        
        switch (os) {
            case TargetOS::Linux:
                paths = {"/usr/include", "/usr/local/include"};
                break;
            case TargetOS::MacOS:
                paths = {"/usr/include", "/usr/local/include", "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include"};
                break;
            case TargetOS::Windows:
                paths = {"C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/include"};
                break;
            case TargetOS::FreeBSD:
                paths = {"/usr/include", "/usr/local/include"};
                break;
            default:
                break;
        }
        
        return paths;
    }

    // Feature detection
    inline bool supportsFeature(TargetArch arch, const std::string& feature) {
        auto features = getArchitectureInfo(arch).features;
        return std::find(features.begin(), features.end(), feature) != features.end();
    }

    inline std::vector<std::string> getAvailableFeatures(TargetArch arch) {
        return getArchitectureInfo(arch).features;
    }

    inline std::string getDefaultCPUForArch(TargetArch arch) {
        switch (arch) {
            case TargetArch::X86_64: return "x86-64";
            case TargetArch::ARM64: return "generic";
            case TargetArch::X86_32: return "i686";
            case TargetArch::ARM32: return "generic";
            case TargetArch::RISCV64: return "generic-rv64";
            case TargetArch::WASM32: case TargetArch::WASM64: return "generic";
            default: return "generic";
        }
    }

    // Compatibility checks
    inline bool isArchitectureCompatible(TargetArch source, TargetArch target) {
        if (source == target) return true;
        
        // x86_64 can run x86_32
        if (source == TargetArch::X86_32 && target == TargetArch::X86_64) return true;
        
        // ARM64 can run ARM32 in some cases
        if (source == TargetArch::ARM32 && target == TargetArch::ARM64) return true;
        
        return false;
    }

    inline bool isOSCompatible(TargetOS source, TargetOS target) {
        if (source == target) return true;
        
        // Basic compatibility rules (expand as needed)
        if ((source == TargetOS::Linux || source == TargetOS::FreeBSD) && 
            (target == TargetOS::Linux || target == TargetOS::FreeBSD)) {
            return true; // Unix-like compatibility
        }
        
        return false;
    }

    // Debug utilities
    inline void printHostInfo() {
        auto hostArch = detectHostArchitecture();
        auto hostOS = detectHostOS();
        auto archInfo = getArchitectureInfo(hostArch);
        auto osInfo = getOSInfo(hostOS);
        
        printf("Host Information:\n");
        printf("  Architecture: %s (%s)\n", archInfo.name.c_str(), archInfo.llvmName.c_str());
        printf("  Pointer Size: %d bytes\n", archInfo.pointerSize);
        printf("  Operating System: %s (%s)\n", osInfo.name.c_str(), osInfo.llvmName.c_str());
        printf("  Target Triple: %s\n", detectHostTriple().c_str());
        printf("  Features: ");
        for (const auto& feature : archInfo.features) {
            printf("%s ", feature.c_str());
        }
        printf("\n");
    }

    inline std::string getTargetSummary(TargetArch arch, TargetOS os) {
        auto archInfo = getArchitectureInfo(arch);
        auto osInfo = getOSInfo(os);
        return archInfo.name + "-" + osInfo.name + " (" + generateTriple(arch, os) + ")";
    }

} // namespace TargetInfo
```

### Explanation
- **Host Detection**:
  - `detectHostArchitecture()`: Uses preprocessor macros to identify the host architecture (e.g., `__x86_64__` for `X86_64`).
  - `detectHostOS()`: Detects the OS using macros like `__linux__` or `__APPLE__`, with conditional checks for iOS vs. macOS.
  - `detectHostVendor()` and `detectHostEnvironment()`: Provide sensible defaults for vendor (e.g., `pc` for Windows, `apple` for macOS) and environment (e.g., `msvc` or `gnu`).
  - `detectHostTriple()`: Combines the above to generate a host triple (e.g., `x86_64-pc-linux-gnu`).
- **Architecture/OS Lookup**:
  - `getArchitectureInfo()`: Returns a static `ArchitectureInfo` struct with details like LLVM name, pointer size, and features (e.g., `sse`, `neon`).
  - `getOSInfo()`: Returns an `OSInfo` struct with OS-specific details, including file extensions and system libraries.
- **Triple Management**:
  - `generateTriple()`: Constructs an LLVM-compatible triple from architecture, OS, vendor, and environment.
  - `TargetTriple::parse()`: Splits a triple string into components, handling edge cases like missing parts.
  - `isValidTriple()`: Validates triples against known architectures and OSes.
  - `normalizeTriple()`: Standardizes triple components (e.g., `x86-64` to `x86_64`, `macos` to `darwin`).
- **Utility Functions**: Provide access to specific metadata (e.g., `getPointerSize()`, `getExecutableExtension()`) and support cross-compilation (e.g., `getDefaultLibraryPaths()`).
- **Compatibility and Features**: Functions like `isArchitectureCompatible()` and `supportsFeature()` ensure correct target configuration.
- **Debug Utilities**: `printHostInfo()` and `getTargetSummary()` provide diagnostic output for target configuration.

## Usage in OS Compiler
The `TargetInfo` component is used during code generation to configure the LLVM backend for the target platform. For example, when compiling the `starfield.os` script:

```cpp
auto triple = TargetInfo::generateTriple(TargetArch::X86_64, TargetOS::Linux);
llvm::InitializeNativeTarget();
llvm::Triple llvmTriple(triple);
auto target = llvm::TargetRegistry::lookupTarget(llvmTriple.str(), error);
```

This sets up LLVM for x86_64 Linux code generation. The compiler might also use:

```cpp
if (TargetInfo::isCrossCompilation(TargetArch::ARM64, TargetOS::Android)) {
    console.warn("Cross-compiling for ARM64-Android, ensure toolchain is available");
}
```

For the `types.os` example, `TargetInfo` provides pointer sizes for type layouts:

```cpp
int ptrSize = TargetInfo::getPointerSize(TargetArch::X86_64); // Returns 8
```

This ensures correct memory layout for types like `Vec2` or `Sprite`.

## Development Notes
The `TargetInfo` component was developed to support cross-compilation and LLVM integration, critical for the OS compiler’s portability. Key design decisions include:
- **Static Lookup Tables**: Using `unordered_map` for `archMap` and `osMap` ensures fast, constant-time access to metadata.
- **Auto Detection**: The `Auto` options in `TargetArch` and `TargetOS` simplify host compilation, while explicit options support cross-compilation.
- **Normalization**: Functions like `normalizeArchName()` handle aliases (e.g., `amd64` to `x86_64`) to ensure compatibility with LLVM.
- **Platform-Specific Code**: Conditional compilation handles diverse platforms, with fallbacks for unknown cases.
Challenges included supporting a wide range of architectures and OSes while maintaining simplicity. The use of inline implementations avoids a separate `.cpp` file, reducing build complexity but increasing header size. The component was designed after `Core` to leverage its utilities (though none are directly used here) and before the LLVM backend to provide target metadata.

## Dependencies
- **Standard Library**: Uses `<string>`, `<vector>`, `<unordered_map>`, `<unordered_set>`, `<algorithm>`, `<sstream>`, `<cstdio>` for data structures and utilities.
- **Platform-Specific**: Requires `<TargetConditionals.h>` for macOS/iOS detection, included only under `__APPLE__`.
- **No OS Components**: `TargetInfo` is standalone, with no direct dependencies on `FileSpan`, `Console`, or `Core`, though it may interact with `Console` for diagnostic output in practice.

## Source Code
- Header: [https://github.com/0m0g1/omniscript/blob/main/include/omniscript/TargetInfo.h](https://github.com/0m0g1/omniscript/blob/main/include/omniscript/TargetInfo.h)
- Implementation: Inline in the header, no separate `.cpp` file.

## Integration with Project
- **File Placement**:
  - Header: `include/omniscript/TargetInfo.h`
- **Build System**: The `premake5.lua` script includes `TargetInfo.h` in the include path. No separate `.cpp` file is needed due to inline implementation. On macOS/iOS, the build system must handle `<TargetConditionals.h>` availability.
- **Compatibility**: Supports Debug/Release modes and integrates with the OS build system, particularly for LLVM-based code generation. No additional libraries are required beyond the standard library.

## Adding to the Index
Add the following entry to your `index.md` under the Component Reference table:

```markdown
| TargetInfo | Manages target architectures, operating systems, and triples for cross-compilation. | [TargetInfo](TargetInfo.md) |
```