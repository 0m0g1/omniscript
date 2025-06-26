#pragma once
<<<<<<< HEAD:include/omniscript/Backends/LLVM/ExternalFunctionResolvers/StaticLibraryLLVMResolver.h
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
=======
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:include/omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/StaticLibraryLLVMResolver.h

// Generic static library resolver
class StaticLibraryResolver : public ExternalFunctionResolver {
private:
    std::string specifiedLibraryPath;

public:
    StaticLibraryResolver();
    
    StaticLibraryResolver(const std::string& libPath);
    
    // Set the library path after construction
    void setLibraryPath(const std::string& libPath);
    
    // Get the current library path
    const std::string& getLibraryPath() const;
    
    llvm::Function* resolve(IRGenerator& generator, const std::string& name,
                           llvm::FunctionType* funcType, LinkDependencies& deps) override;
    
    // Utility function to validate library path
    bool isValidLibraryPath(const std::string& libPath) const;
    
    // Generate linker command preview
    std::string generateLinkerCommand(const LinkDependencies& deps) const;
};