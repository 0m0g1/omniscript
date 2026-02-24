#pragma once
#include <omniscript/backends/llvm/LLVMExternalFunctionResolver.h>

// Windows-specific resolvers
namespace Omniscript {
class WindowsAPIResolver : public ExternalFunctionResolver {
public:
    WindowsAPIResolver(); // Default constructor
    WindowsAPIResolver(const std::string& libraryPath);
    llvm::Function* resolve(IRGenerator& generator, const std::string& name,
                           llvm::FunctionType* funcType, LinkDependencies& deps) override;
    
    // Check if a library path represents a Windows system library
    static bool isWindowsSystemLibrary(const std::string& libraryPath);
    
    // Check if a function can be resolved by trying to load the specified library
    static bool isLikelyWindowsAPIFunction(const std::string& name);
    static std::string getRequiredLibraryForFunction(const std::string& name);
    
private:
    std::string specifiedLibraryPath;

    static bool canResolveFunction(const std::string& functionName, const std::string& libraryPath);
    static void setCorrectCallingConvention(llvm::Function* func, const std::string& name);
    static void addFunctionAttributes(llvm::Function* func, const std::string& name);
    static bool isCRuntimeFunction(const std::string& name);
    static bool isWindowsSystemLibraryName(const std::string& libraryName);
    static std::string normalizeLibraryPath(const std::string& path);
    static bool functionExistsInLibrary(const std::string& functionName, const std::string& libraryPath);

};

} // namespace Omniscript
