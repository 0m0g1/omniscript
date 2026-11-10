#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/DynamicLibraryLLVMResolver.h>
#include <filesystem>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace Omniscript {
namespace {
// Directory containing the running engine executable ("" if unknown).
std::filesystem::path executableDir() {
    namespace fs = std::filesystem;
#ifdef _WIN32
    char buf[MAX_PATH * 4];
    DWORD n = GetModuleFileNameA(nullptr, buf, sizeof buf);
    if (n > 0 && n < sizeof buf) return fs::path(std::string(buf, n)).parent_path();
#elif defined(__linux__)
    std::error_code ec;
    auto p = fs::read_symlink("/proc/self/exe", ec);
    if (!ec) return p.parent_path();
#endif
    return {};
}

// Resolve a relative path by walking UP from `start` (e.g. bin/Debug-x/ -> bin/ -> project root)
// and returning the first ancestor where `rel` exists.
std::string findUpwards(std::filesystem::path start, const std::string& rel) {
    namespace fs = std::filesystem;
    std::error_code ec;
    for (; !start.empty(); ) {
        fs::path candidate = start / rel;
        if (fs::exists(candidate, ec)) return candidate.lexically_normal().string();
        fs::path parent = start.parent_path();
        if (parent == start) break;
        start = parent;
    }
    return {};
}
} // namespace

DynamicLibraryResolver::DynamicLibraryResolver(const std::string& libPath) 
    : libPath_(normalizePath(libPath)) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const bool hasDirectory = libPath_.find_first_of("/\\") != std::string::npos;
    const bool fileExists = fs::exists(libPath_, ec);

    // A path with a directory part that does not exist is a path problem
    // (usually a wrong working directory), not a missing-dependency problem.
    if (hasDirectory && !fileExists) {
        console.reportError(Console::LINK_ERROR,
            "Dynamic library not found: '" + libPath_ +
            "' (relative paths are resolved from the working directory: " +
            fs::current_path(ec).string() + ")", FileSpan{});
        return;
    }

    unsigned long preloadError = 0;
#ifdef _WIN32
    // LLVM loads with plain LoadLibrary, which does not search the DLL's own
    // folder for ITS dependencies (e.g. libcurl -> libssl/libcrypto/zlib).
    // Pre-load with LOAD_WITH_ALTERED_SEARCH_PATH so sibling DLLs are found.
    if (hasDirectory && fileExists && fs::path(libPath_).is_absolute()) {
        if (!LoadLibraryExA(libPath_.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH)) {
            preloadError = GetLastError();
        }
    }
#endif

    std::string error;
    dynLib = llvm::sys::DynamicLibrary::getPermanentLibrary(libPath_.c_str(), &error);
    if (!dynLib.isValid()) {
        std::string msg = "Failed to load dynamic library '" + libPath_ + "': " + error;
        if (hasDirectory && fileExists) {
            msg += " (the file exists, so a DLL it depends on is probably missing or has the wrong architecture";
            if (preloadError) msg += "; Win32 error " + std::to_string(preloadError);
            msg += ")";
        }
        console.reportError(Console::LINK_ERROR, msg, FileSpan{});
    }
}

llvm::Function* DynamicLibraryResolver::resolve(IRGenerator& generator, const std::string& name, 
                                              llvm::FunctionType* funcType, LinkDependencies& deps) {
    // Always emit the declaration so the module stays valid and the JIT pre-flight
    // check (LLVMJITBackend) can refuse to run it, instead of a null call at runtime.
    llvm::Function* func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, name, generator.getCurrentModule()
    );

    if (dynLib.isValid() && !dynLib.getAddressOfSymbol(name.c_str())) {
        console.reportError(Console::LINK_ERROR, "Symbol '" + name + "' does not exist in the library '" + libPath_ + "'.", FileSpan{});
    }
    
    // Add library dependency
    LinkDependencies::LibraryInfo info;
    info.name = libPath_;
    info.path = libPath_;
    deps.addRequiredLibrary(libPath_, info);
    
    return func;
}

std::string DynamicLibraryResolver::normalizePath(const std::string& path) {
    // Linkage-only externs have no library path; don't append extensions to "".
    if (path.empty() || path == "C" || path == "C++" ||
        path == "c" || path == "c++") {
        return "";
    }
    // Logical system libraries: extern "@libm" { ... } picks the right file per platform, so one
    // std source file works everywhere. Returned as-is (bare names go through the system loader).
    //   @libm : the C math library      @libc : the C runtime
    if (!path.empty() && path[0] == '@') {
        const std::string logical = path.substr(1);
        PlatformInfo::Platform current = PlatformInfo::getCurrentPlatform();
        if (logical == "libm" || logical == "libc") {
            if (current == PlatformInfo::Windows) return "ucrtbase.dll";          // UCRT exports C99 math + CRT
            if (current == PlatformInfo::MacOS)   return "libSystem.B.dylib";     // libm/libc live in libSystem
            return logical == "libm" ? "libm.so.6" : "libc.so.6";                 // not "libm.so": that is a linker script on glibc
        }
        // Unknown logical name: fall through so the normal "library not found" error names it.
    }

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
    
    // Make paths with a directory part absolute, but only when the file exists.
    // Bare names ("kernel32.dll", "libc.so.6") must stay as-is so the system
    // loader can search for them.
    if (normalized.find_first_of("/\\") != std::string::npos) {
        namespace fs = std::filesystem;
        std::error_code ec;
        if (fs::path(normalized).is_absolute()) {
            if (fs::exists(normalized, ec)) normalized = fs::path(normalized).lexically_normal().string();
        } else {
            // 1) working directory (old behaviour)
            auto abs = fs::absolute(normalized, ec);
            if (!ec && fs::exists(abs, ec)) {
                normalized = abs.lexically_normal().string();
            } else {
                // 2) walk up from the engine executable: finds <project>/dependencies/...
                //    from <project>/bin/Debug-windows-x86_64/Osengine.exe
                std::string found = findUpwards(executableDir(), normalized);
                // 3) walk up from the working directory (when run from a subfolder)
                if (found.empty()) found = findUpwards(fs::current_path(ec), normalized);
                if (!found.empty()) normalized = found;
            }
        }
    }

    return normalized;
}

} // namespace Omniscript