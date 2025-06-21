#include <omniscript/engine/Backends/LLVM/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/WindowsAPILLVMResolver.h>

#ifdef _WIN32
#include <windows.h>
#include <libloaderapi.h>
#include <shlobj.h>
#endif

// Static member initialization
bool WindowsAPIResolver::isScanned = false;
std::unordered_set<std::string> WindowsAPIResolver::discoveredLibraries;
std::unordered_set<std::string> WindowsAPIResolver::availableDlls;
std::unordered_set<std::string> WindowsAPIResolver::availableStaticLibs;

void WindowsAPIResolver::scanWindowsDirectories() {
    if (isScanned) return;
    
#ifdef _WIN32
    std::vector<std::string> systemDirs = getWindowsSystemDirectories();
    std::vector<std::string> dllExtensions = {".dll"};
    std::vector<std::string> libExtensions = {".lib", ".a"};
    
    // Scan for DLLs
    for (const std::string& dir : systemDirs) {
        scanDirectory(dir, dllExtensions);
    }
    
    // Scan for static libraries in additional directories
    std::vector<std::string> additionalLibDirs;
    
    // Add Windows SDK lib directories
    char* programFiles = nullptr;
    size_t len = 0;
    if (_dupenv_s(&programFiles, &len, "ProgramFiles") == 0 && programFiles) {
        std::string sdkPath = std::string(programFiles) + "\\Windows Kits";
        if (std::filesystem::exists(sdkPath)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(sdkPath)) {
                if (entry.is_directory() && entry.path().filename() == "lib") {
                    additionalLibDirs.push_back(entry.path().string());
                }
            }
        }
        free(programFiles);
    }
    
    // Add Visual Studio lib directories
    char* programFilesX86 = nullptr;
    if (_dupenv_s(&programFilesX86, &len, "ProgramFiles(x86)") == 0 && programFilesX86) {
        std::string vsPath = std::string(programFilesX86) + "\\Microsoft Visual Studio";
        if (std::filesystem::exists(vsPath)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(vsPath)) {
                if (entry.is_directory() && entry.path().filename() == "lib") {
                    additionalLibDirs.push_back(entry.path().string());
                }
            }
        }
        free(programFilesX86);
    }
    
    // Scan additional directories for static libraries
    for (const std::string& dir : additionalLibDirs) {
        scanDirectory(dir, libExtensions);
    }
    
#endif
    
    isScanned = true;
}

std::vector<std::string> WindowsAPIResolver::getWindowsSystemDirectories() {
    std::vector<std::string> directories;
    
#ifdef _WIN32
    char buffer[MAX_PATH];
    
    // System32 directory
    if (GetSystemDirectoryA(buffer, MAX_PATH)) {
        directories.push_back(std::string(buffer));
    }
    
    // SysWOW64 directory (for 32-bit libraries on 64-bit systems)
    if (GetSystemWow64DirectoryA(buffer, MAX_PATH)) {
        directories.push_back(std::string(buffer));
    }
    
    // Windows directory
    if (GetWindowsDirectoryA(buffer, MAX_PATH)) {
        directories.push_back(std::string(buffer));
        directories.push_back(std::string(buffer) + "\\System");
    }
    
    // Current directory (sometimes libraries are here)
    if (GetCurrentDirectoryA(MAX_PATH, buffer)) {
        directories.push_back(std::string(buffer));
    }
    
    // PATH directories
    char* pathEnv = nullptr;
    size_t len = 0;
    if (_dupenv_s(&pathEnv, &len, "PATH") == 0 && pathEnv) {
        std::string pathStr(pathEnv);
        size_t start = 0;
        size_t end = pathStr.find(';');
        
        while (end != std::string::npos) {
            std::string dir = pathStr.substr(start, end - start);
            if (!dir.empty() && std::filesystem::exists(dir)) {
                directories.push_back(dir);
            }
            start = end + 1;
            end = pathStr.find(';', start);
        }
        
        // Add the last directory
        std::string lastDir = pathStr.substr(start);
        if (!lastDir.empty() && std::filesystem::exists(lastDir)) {
            directories.push_back(lastDir);
        }
        
        free(pathEnv);
    }
#endif
    
    return directories;
}

void WindowsAPIResolver::scanDirectory(const std::string& directory, const std::vector<std::string>& extensions) {
#ifdef _WIN32
    try {
        if (!std::filesystem::exists(directory)) {
            return;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filePath = entry.path().string();
                std::string extension = entry.path().extension().string();
                
                // Convert extension to lowercase for comparison
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                // Check if this file has one of the desired extensions
                for (const std::string& ext : extensions) {
                    if (extension == ext) {
                        std::string baseName = extractLibraryBaseName(filePath);
                        if (!baseName.empty()) {
                            discoveredLibraries.insert(baseName);
                            
                            if (ext == ".dll") {
                                availableDlls.insert(baseName);
                            } else if (ext == ".lib" || ext == ".a") {
                                availableStaticLibs.insert(baseName);
                            }
                        }
                        break;
                    }
                }
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        // Ignore directories we can't access
    }
#endif
}

std::string WindowsAPIResolver::extractLibraryBaseName(const std::string& filePath) {
#ifdef _WIN32
    std::filesystem::path path(filePath);
    std::string filename = path.filename().string();
    std::string stem = path.stem().string();
    
    // Convert to lowercase for consistency
    std::transform(stem.begin(), stem.end(), stem.begin(), ::tolower);
    
    // Filter out obvious non-system libraries and temporary files
    if (stem.empty() || 
        stem.find("temp") != std::string::npos ||
        stem.find("tmp") != std::string::npos ||
        stem.find("cache") != std::string::npos ||
        stem.size() < 3) {
        return "";
    }
    
    return stem;
#else
    return "";
#endif
}

llvm::Function* WindowsAPIResolver::resolve(IRGenerator& generator, const std::string& name, 
                                          llvm::FunctionType* funcType, LinkDependencies& deps) {
    if (PlatformInfo::getCurrentPlatform() != PlatformInfo::Windows) {
        return nullptr; // Only resolve on Windows
    }
    
    std::string requiredLibrary = findLibraryForFunction(name);
    if (requiredLibrary.empty()) {
        return nullptr; // Function not found in any Windows library
    }
    
    // Create the function
    llvm::Function* func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, name, generator.getCurrentModule()
    );
    
    // Set Windows calling convention (usually stdcall for Win32 API)
    func->setCallingConv(llvm::CallingConv::X86_StdCall);
    
    // Add library dependency
    LinkDependencies::LibraryInfo info;
    info.name = requiredLibrary + ".dll";
    info.isSystemLib = true;
    deps.addRequiredLibrary(requiredLibrary + ".dll", info);
    
    return func;
}

bool WindowsAPIResolver::isWindowsSystemLibrary(const std::string& libraryName) {
    scanWindowsDirectories();
    
    // Remove common file extensions for comparison
    std::string baseName = libraryName;
    if (baseName.size() > 4 && baseName.substr(baseName.size() - 4) == ".dll") {
        baseName = baseName.substr(0, baseName.size() - 4);
    }
    if (baseName.size() > 4 && baseName.substr(baseName.size() - 4) == ".lib") {
        baseName = baseName.substr(0, baseName.size() - 4);
    }
    
    // Convert to lowercase for comparison
    std::transform(baseName.begin(), baseName.end(), baseName.begin(), ::tolower);
    
    return discoveredLibraries.find(baseName) != discoveredLibraries.end();
}

bool WindowsAPIResolver::canResolveFunction(const std::string& functionName) {
    return !findLibraryForFunction(functionName).empty();
}

std::string WindowsAPIResolver::findLibraryForFunction(const std::string& functionName) {
    scanWindowsDirectories();
    
#ifdef _WIN32
    // First try DLLs (runtime libraries)
    for (const std::string& libName : availableDlls) {
        if (functionExistsInLibrary(functionName, libName)) {
            return libName;
        }
    }
#endif
    
    return "";
}

bool WindowsAPIResolver::functionExistsInLibrary(const std::string& functionName, const std::string& libraryName) {
#ifdef _WIN32
    std::string dllName = libraryName + ".dll";
    
    // Try to load the library
    HMODULE hModule = LoadLibraryA(dllName.c_str());
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

const std::unordered_set<std::string>& WindowsAPIResolver::getDiscoveredWindowsLibraries() {
    scanWindowsDirectories();
    return discoveredLibraries;
}

void WindowsAPIResolver::refreshLibraryCache() {
    isScanned = false;
    discoveredLibraries.clear();
    availableDlls.clear();
    availableStaticLibs.clear();
    scanWindowsDirectories();
}