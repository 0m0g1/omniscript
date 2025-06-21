#pragma once
#include <omniscript/engine/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <unordered_set>
#include <vector>
#include <string>

// Windows-specific resolvers
class WindowsAPIResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name,
                           llvm::FunctionType* funcType, LinkDependencies& deps) override;
    
    // Check if a library name represents a Windows system library (dynamic check)
    static bool isWindowsSystemLibrary(const std::string& libraryName);
    
    // Check if a function can be resolved by querying actual Windows libraries
    static bool canResolveFunction(const std::string& functionName);
    
    // Try to find which Windows library contains the function
    static std::string findLibraryForFunction(const std::string& functionName);
    
    // Get all discovered Windows system libraries
    static const std::unordered_set<std::string>& getDiscoveredWindowsLibraries();
    
    // Force rescan of Windows directories
    static void refreshLibraryCache();

private:
    // Scan Windows system directories for libraries
    static void scanWindowsDirectories();
    
    // Helper functions for directory scanning
    static void scanDirectory(const std::string& directory, const std::vector<std::string>& extensions);
    static std::vector<std::string> getWindowsSystemDirectories();
    static std::string extractLibraryBaseName(const std::string& filePath);
    
    // Helper to check if function exists in a specific library
    static bool functionExistsInLibrary(const std::string& functionName, const std::string& libraryName);
    
    // Static data
    static bool isScanned;
    static std::unordered_set<std::string> discoveredLibraries;
    static std::unordered_set<std::string> availableDlls;
    static std::unordered_set<std::string> availableStaticLibs;
};