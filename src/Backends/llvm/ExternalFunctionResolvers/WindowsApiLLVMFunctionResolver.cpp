#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/WindowsAPILLVMResolver.h>

#ifdef _WIN32
#include <libloaderapi.h>
#endif

WindowsAPIResolver::WindowsAPIResolver() {
}

WindowsAPIResolver::WindowsAPIResolver(const std::string& libraryPath) 
    : specifiedLibraryPath(libraryPath) {
}

llvm::Function* WindowsAPIResolver::resolve(IRGenerator& generator, const std::string& name, 
                                          llvm::FunctionType* funcType, LinkDependencies& deps) {
    if (PlatformInfo::getCurrentPlatform() != PlatformInfo::Windows) {
        return nullptr; // Only resolve on Windows
    }
    
    // Determine the library name - use specified path if available, otherwise auto-detect
    std::string libName;
    if (!specifiedLibraryPath.empty()) {
        // Extract library name from path
        std::filesystem::path path(specifiedLibraryPath);
        libName = path.stem().string(); // Remove .lib/.dll extension
    } else {
        libName = getRequiredLibraryForFunction(name);
    }
    
    // Create the function
    llvm::Function* func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, name, generator.getCurrentModule()
    );
    
    // Set calling convention based on architecture
    if (sizeof(void*) == 8) {
        // x64 Windows uses unified calling convention
        func->setCallingConv(llvm::CallingConv::C);
        // func->setCallingConv(llvm::CallingConv::X86_StdCall);
    } else {
        // x86 Windows - most Win32 API uses stdcall
        func->setCallingConv(llvm::CallingConv::X86_StdCall);
    }

    // Add DLL import storage class for better linking
    // func->setDLLStorageClass(llvm::GlobalValue::DLLImportStorageClass);

    // Add library dependency
    LinkDependencies::LibraryInfo info;
    info.name = libName;
    info.isSystemLib = true;
    deps.addRequiredLibrary(libName, info);
    
    return func;
}

std::string WindowsAPIResolver::getRequiredLibraryForFunction(const std::string& name) {
    // Map common functions to their required libraries
    static const std::unordered_map<std::string, std::string> functionToLibrary = {
        // Kernel32.dll functions
        {"GetModuleHandle", "kernel32"}, {"GetModuleHandleA", "kernel32"}, {"GetModuleHandleW", "kernel32"},
        {"LoadLibrary", "kernel32"}, {"LoadLibraryA", "kernel32"}, {"LoadLibraryW", "kernel32"},
        {"GetProcAddress", "kernel32"},
        {"CreateFile", "kernel32"}, {"CreateFileA", "kernel32"}, {"CreateFileW", "kernel32"},
        {"ReadFile", "kernel32"}, {"WriteFile", "kernel32"}, {"CloseHandle", "kernel32"},
        {"GetSystemDirectory", "kernel32"}, {"GetSystemDirectoryA", "kernel32"}, {"GetSystemDirectoryW", "kernel32"},
        {"GetWindowsDirectory", "kernel32"}, {"GetWindowsDirectoryA", "kernel32"}, {"GetWindowsDirectoryW", "kernel32"},
        {"Sleep", "kernel32"}, {"GetTickCount", "kernel32"}, {"GetCurrentProcess", "kernel32"},
        {"GetCurrentThread", "kernel32"}, {"ExitProcess", "kernel32"},
        
        // User32.dll functions
        {"MessageBox", "user32"}, {"MessageBoxA", "user32"}, {"MessageBoxW", "user32"},
        {"FindWindow", "user32"}, {"FindWindowA", "user32"}, {"FindWindowW", "user32"},
        {"GetWindowText", "user32"}, {"GetWindowTextA", "user32"}, {"GetWindowTextW", "user32"},
        {"ShowWindow", "user32"}, {"UpdateWindow", "user32"}, {"GetDC", "user32"}, {"ReleaseDC", "user32"},
        
        // GDI32.dll functions
        {"CreateDC", "gdi32"}, {"CreateDCA", "gdi32"}, {"CreateDCW", "gdi32"},
        {"DeleteDC", "gdi32"}, {"BitBlt", "gdi32"}, {"CreateBitmap", "gdi32"},
        
        // WS2_32.dll functions
        {"WSAStartup", "ws2_32"}, {"WSACleanup", "ws2_32"}, {"socket", "ws2_32"},
        {"connect", "ws2_32"}, {"send", "ws2_32"}, {"recv", "ws2_32"}, {"closesocket", "ws2_32"},
        
        // Shell32.dll functions
        {"ShellExecute", "shell32"}, {"ShellExecuteA", "shell32"}, {"ShellExecuteW", "shell32"},
        {"SHGetFolderPath", "shell32"}, {"SHGetFolderPathA", "shell32"}, {"SHGetFolderPathW", "shell32"},
        
        // Advapi32.dll functions
        {"RegOpenKeyEx", "advapi32"}, {"RegOpenKeyExA", "advapi32"}, {"RegOpenKeyExW", "advapi32"},
        {"RegCloseKey", "advapi32"}, {"RegQueryValueEx", "advapi32"}, {"RegQueryValueExA", "advapi32"}, {"RegQueryValueExW", "advapi32"}
    };
    
    auto it = functionToLibrary.find(name);
    if (it != functionToLibrary.end()) {
        return it->second;
    }
    
    // Default fallback based on function prefixes
    if (name.find("Wsa") == 0 || name.find("WSA") == 0) {
        return "ws2_32";
    } else if (name.find("Reg") == 0) {
        return "advapi32";
    } else if (name.find("Shell") == 0 || name.find("SH") == 0) {
        return "shell32";
    } else if (name.find("MessageBox") == 0 || name.find("FindWindow") == 0 || name.find("ShowWindow") == 0) {
        return "user32";
    } else if (name.find("CreateDC") == 0 || name.find("DeleteDC") == 0 || name.find("BitBlt") == 0) {
        return "gdi32";
    }
    
    // Most common default for Windows API functions
    return "kernel32";
}

bool WindowsAPIResolver::isLikelyWindowsAPIFunction(const std::string& name) {
    // Check if function name matches common Windows API patterns
    static const std::vector<std::string> windowsApiPrefixes = {
        "Get", "Set", "Create", "Delete", "Open", "Close", "Read", "Write",
        "Find", "Load", "Free", "Query", "Reg", "Shell", "Show", "Update",
        "Message", "Window", "File", "Handle", "Process", "Thread", "WSA"
    };
    
    for (const auto& prefix : windowsApiPrefixes) {
        if (name.find(prefix) == 0) {
            return true;
        }
    }
    
    // Check for common Windows API suffixes
    if (name.length() > 1) {
        char lastChar = name.back();
        if (lastChar == 'A' || lastChar == 'W') {
            std::string baseName = name.substr(0, name.length() - 1);
            for (const auto& prefix : windowsApiPrefixes) {
                if (baseName.find(prefix) == 0) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool WindowsAPIResolver::isWindowsSystemLibrary(const std::string& libraryPath) {
    if (libraryPath.empty()) {
        return false;
    }
    
    std::string normalizedPath = normalizeLibraryPath(libraryPath);
    
    // Check if it's in a Windows system directory
#ifdef _WIN32
    char systemDir[MAX_PATH];
    char windowsDir[MAX_PATH];
    char sysWow64Dir[MAX_PATH];
    
    std::vector<std::string> systemDirs;
    
    if (GetSystemDirectoryA(systemDir, MAX_PATH)) {
        systemDirs.push_back(std::string(systemDir));
    }
    
    if (GetWindowsDirectoryA(windowsDir, MAX_PATH)) {
        systemDirs.push_back(std::string(windowsDir));
        systemDirs.push_back(std::string(windowsDir) + "\\System");
    }
    
    if (GetSystemWow64DirectoryA(sysWow64Dir, MAX_PATH)) {
        systemDirs.push_back(std::string(sysWow64Dir));
    }
    
    // Check if the library path is in any system directory
    for (const std::string& sysDir : systemDirs) {
        std::string normalizedSysDir = sysDir;
        std::transform(normalizedSysDir.begin(), normalizedSysDir.end(), normalizedSysDir.begin(), ::tolower);
        
        if (normalizedPath.find(normalizedSysDir) == 0) {
            return true;
        }
    }
#endif
    
    // Also check by library name (for cases where user just provides "kernel32.dll")
    std::filesystem::path path(libraryPath);
    std::string filename = path.filename().string();
    std::string stem = path.stem().string();
    
    return isWindowsSystemLibraryName(stem);
}

bool WindowsAPIResolver::isWindowsSystemLibraryName(const std::string& libraryName) {
    static const std::unordered_set<std::string> windowsSystemLibs = {
        // Core Windows libraries
        "kernel32", "user32", "gdi32", "advapi32", "shell32", "ws2_32",
        "ole32", "oleaut32", "comctl32", "comdlg32", "winmm", "version",
        "imagehlp", "psapi", "netapi32", "winspool", "crypt32", "secur32",
        "ntdll", "msvcrt", "ucrtbase", "vcruntime140", "msvcp140",
        
        // Graphics and multimedia
        "opengl32", "glu32", "d3d9", "d3d11", "dxgi", "d2d1", "dwrite",
        "wincodecs", "mf", "mfplat", "evr",
        
        // Network and internet
        "wininet", "urlmon", "winhttp", "iphlpapi", "dnsapi",
        
        // System utilities
        "shlwapi", "mpr", "wintrust", "cabinet", "setupapi", "cfgmgr32",
        "powrprof", "userenv", "authz", "wtsapi32",
        
        // Input and HID
        "hid", "xinput1_4", "xinput9_1_0", "dinput8",
        
        // Common runtime libraries
        "msvcr120", "msvcr110", "msvcr100", "msvcr90", "msvcr80",
        "msvcp120", "msvcp110", "msvcp100", "msvcp90", "msvcp80"
    };
    
    std::string lowerName = libraryName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    return windowsSystemLibs.find(lowerName) != windowsSystemLibs.end();
}

std::string WindowsAPIResolver::normalizeLibraryPath(const std::string& path) {
    std::string normalized = path;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Replace forward slashes with backslashes on Windows
#ifdef _WIN32
    std::replace(normalized.begin(), normalized.end(), '/', '\\');
#endif
    
    return normalized;
}

bool WindowsAPIResolver::canResolveFunction(const std::string& functionName, const std::string& libraryPath) {
    if (!isWindowsSystemLibrary(libraryPath)) {
        return false;
    }
    
    return functionExistsInLibrary(functionName, libraryPath);
}

bool WindowsAPIResolver::functionExistsInLibrary(const std::string& functionName, const std::string& libraryPath) {
#ifdef _WIN32
    // Extract just the library name from the path
    std::filesystem::path path(libraryPath);
    std::string libName = path.filename().string();
    
    // Ensure it has .dll extension
    if (path.extension() != ".dll") {
        std::string stem = path.stem().string();
        libName = stem + ".dll";
    }
    
    // Try to load the library
    HMODULE hModule = LoadLibraryA(libName.c_str());
    if (hModule == nullptr) {
        return false;
    }
    
    // Check if the function exists
    FARPROC proc = GetProcAddress(hModule, functionName.c_str());
    bool exists = (proc != nullptr);
    
    // Free the library
    FreeLibrary(hModule);
    
    return exists;
#else
    // Can't check on non-Windows platforms
    return false;
#endif
}