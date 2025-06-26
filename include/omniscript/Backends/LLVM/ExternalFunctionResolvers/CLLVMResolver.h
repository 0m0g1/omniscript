#pragma once
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>

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