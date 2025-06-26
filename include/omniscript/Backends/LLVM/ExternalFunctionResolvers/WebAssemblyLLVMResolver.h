#pragma once
<<<<<<< HEAD:include/omniscript/Backends/LLVM/ExternalFunctionResolvers/WebAssemblyLLVMResolver.h
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
=======
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:include/omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/WebAssemblyLLVMResolver.h

// WebAssembly resolver
class WebAssemblyResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, 
                          llvm::FunctionType* funcType, LinkDependencies& deps) override;
    
private:
    bool isWASIFunction(const std::string& name);
    bool isEmscriptenFunction(const std::string& name);
    bool isWebAPIFunction(const std::string& name);
    void applyWasmAttributes(llvm::Function* func, const std::string& name);
};