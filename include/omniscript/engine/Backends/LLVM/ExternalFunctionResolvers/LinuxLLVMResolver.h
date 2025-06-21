#pragma once
#include <omniscript/engine/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/LinuxLLVMResolver.h>

// Linux-specific resolver
class LinuxResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, 
                          llvm::FunctionType* funcType, LinkDependencies& deps) override;
    
private:
    bool isGlibcFunction(const std::string& name);
    bool isSystemCallWrapper(const std::string& name);
};