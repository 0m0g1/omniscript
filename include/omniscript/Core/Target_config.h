#pragma once
#include <omniscript/omniscript_pch.h>

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

    inline std::string detectHostTriple() {
        return generateTriple(detectHostArchitecture(), detectHostOS());
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
        triple.vendor = vendor.empty() ? "unknown" : vendor;
        triple.os = getLLVMOSName(os);
        triple.environment = environment;
        return triple.toString();
    }

    inline bool isValidTriple(const std::string& triple) {
        if (triple.empty()) {
            return false;
        }
        
        // Parse the triple
        TargetTriple parsed = TargetTriple::parse(triple);
        
        // Check if architecture is valid
        static const std::unordered_set<std::string> validArchs = {
            "x86_64", "x86-64", "amd64", "x64",
            "aarch64", "arm64",
            "i386", "i486", "i586", "i686", "x86",
            "arm", "armv6", "armv7",
            "riscv64",
            "wasm32", "wasm64", "wasm"
        };
        
        if (parsed.arch.empty() || validArchs.find(parsed.arch) == validArchs.end()) {
            return false;
        }
        
        // Check if OS is valid (if specified)
        if (!parsed.os.empty()) {
            static const std::unordered_set<std::string> validOSs = {
                "linux", "windows", "darwin", "macos", "freebsd", 
                "android", "ios", "wasm", "unknown"
            };
            
            if (validOSs.find(parsed.os) == validOSs.end()) {
                return false;
            }
        }
        
        // Check vendor (if specified) - most common vendors
        if (!parsed.vendor.empty()) {
            static const std::unordered_set<std::string> validVendors = {
                "unknown", "pc", "apple", "microsoft", "gnu", "android", "none"
            };
            
            if (validVendors.find(parsed.vendor) == validVendors.end()) {
                return false;
            }
        }
        
        // Basic format validation - should not have more than 4 parts
        size_t dashCount = std::count(triple.begin(), triple.end(), '-');
        if (dashCount > 3) {
            return false;
        }
        
        return true;
    }

    inline TargetTriple normalizeTriple(const std::string& triple) {
        TargetTriple parsed = TargetTriple::parse(triple);
        TargetTriple normalized;
        
        // Normalize architecture names
        std::string arch = parsed.arch;
        std::transform(arch.begin(), arch.end(), arch.begin(), ::tolower);
        
        if (arch == "x86-64" || arch == "amd64" || arch == "x64") {
            normalized.arch = "x86_64";
        } else if (arch == "aarch64") {
            normalized.arch = "aarch64";
        } else if (arch == "arm64") {
            normalized.arch = "aarch64";  // Normalize arm64 to aarch64
        } else if (arch == "i386" || arch == "i486" || arch == "i586" || arch == "i686") {
            normalized.arch = "i386";
        } else if (arch == "armv6" || arch == "armv7") {
            normalized.arch = "arm";
        } else if (arch == "wasm") {
            normalized.arch = "wasm32";  // Default wasm to wasm32
        } else {
            normalized.arch = arch;  // Keep as-is if no normalization needed
        }
        
        // Normalize vendor
        if (parsed.vendor.empty() || parsed.vendor == "pc") {
            normalized.vendor = "unknown";
        } else {
            normalized.vendor = parsed.vendor;
        }
        
        // Normalize OS names
        std::string os = parsed.os;
        std::transform(os.begin(), os.end(), os.begin(), ::tolower);
        
        if (os == "darwin" || os == "macos") {
            normalized.os = "darwin";
        } else if (os == "win32" || os == "mingw32" || os == "cygwin") {
            normalized.os = "windows";
        } else if (os.empty()) {
            normalized.os = "unknown";
        } else {
            normalized.os = os;
        }
        
        // Normalize environment
        if (parsed.environment.empty()) {
            // Set default environment based on OS
            if (normalized.os == "linux") {
                normalized.environment = "gnu";
            } else if (normalized.os == "windows") {
                normalized.environment = "msvc";
            } else {
                normalized.environment = "";
            }
        } else {
            normalized.environment = parsed.environment;
        }
        
        return normalized;
    }

    inline TargetTriple TargetTriple::parse(const std::string& triple) {
        TargetTriple result;
        std::vector<std::string> parts;
        std::string current;
        
        for (char c : triple) {
            if (c == '-') {
                if (!current.empty()) {
                    parts.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (!current.empty()) {
            parts.push_back(current);
        }
        
        if (parts.size() >= 1) result.arch = parts[0];
        if (parts.size() >= 2) result.vendor = parts[1];
        if (parts.size() >= 3) result.os = parts[2];
        if (parts.size() >= 4) result.environment = parts[3];
        
        return result;
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

}
