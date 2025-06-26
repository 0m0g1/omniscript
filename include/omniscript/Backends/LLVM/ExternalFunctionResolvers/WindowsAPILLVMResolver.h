#pragma once
<<<<<<< HEAD:include/omniscript/Backends/LLVM/ExternalFunctionResolvers/WindowsAPILLVMResolver.h
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
=======
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:include/omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/WindowsAPILLVMResolver.h

// Windows-specific resolvers
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