#pragma once
#include <omniscript/backends/llvm/LLVMExternalFunctionResolver.h>
#include <omniscript/backends/llvm/ExternalFunctionResolvers/LinuxLLVMResolver.h>

// Linux-specific resolver
namespace Omniscript {
class LinuxResolver : public ExternalFunctionResolver {
public:
    LinuxResolver(); // Default constructor
    LinuxResolver(const std::string& libraryPath);
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, 
                          llvm::FunctionType* funcType, LinkDependencies& deps) override;
    
private:
    std::string specifiedLibraryPath;
    bool isGlibcFunction(const std::string& name);
    bool isSystemCallWrapper(const std::string& name);
};

} // namespace Omniscript
