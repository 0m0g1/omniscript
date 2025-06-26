#pragma once
<<<<<<< HEAD:include/omniscript/Backends/LLVM/ExternalFunctionResolvers/LinuxLLVMResolver.h
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/LinuxLLVMResolver.h>
=======
#include <omniscript/engine/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/LinuxLLVMResolver.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:include/omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/LinuxLLVMResolver.h

// Linux-specific resolver
class LinuxResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, 
                          llvm::FunctionType* funcType, LinkDependencies& deps) override;
    
private:
    bool isGlibcFunction(const std::string& name);
    bool isSystemCallWrapper(const std::string& name);
};