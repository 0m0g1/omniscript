#pragma once
<<<<<<< HEAD:include/omniscript/Backends/LLVM/ExternalFunctionResolvers/AndroidLLVMResolver.h
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
=======
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:include/omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/AndroidLLVMResolver.h

// Android resolver
class AndroidResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, 
                          llvm::FunctionType* funcType, LinkDependencies& deps) override;
    
private:
    bool isAndroidFunction(const std::string& name);
    bool isBionicFunction(const std::string& name);
    bool isJNIFunction(const std::string& name);
    std::string getRequiredLibrary(const std::string& name);
};
