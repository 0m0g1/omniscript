#pragma once
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>

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