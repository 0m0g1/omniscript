#pragma once
#include <omniscript/engine/Backends/LLVM/LLVMExternalFunctionResolver.h>

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