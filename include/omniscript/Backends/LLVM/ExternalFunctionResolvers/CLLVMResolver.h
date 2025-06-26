#pragma once
<<<<<<< HEAD:include/omniscript/Backends/LLVM/ExternalFunctionResolvers/CLLVMResolver.h
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
=======
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:include/omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/CLLVMResolver.h

// Universal C Standard Library Resolver
class CStdLibResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, 
                          llvm::FunctionType* funcType, LinkDependencies& deps) override;
    static bool isCStdLibFunction(const std::string& name);
    
private:
    void applyPlatformSpecificAttributes(llvm::Function* func, const std::string& name);
    llvm::CallingConv::ID getPlatformCallingConv(const std::string& name);
};