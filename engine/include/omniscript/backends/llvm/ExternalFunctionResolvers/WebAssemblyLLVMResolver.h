#pragma once
#include <omniscript/backends/llvm/LLVMExternalFunctionResolver.h>

// WebAssembly resolver
namespace Omniscript {
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

} // namespace Omniscript
