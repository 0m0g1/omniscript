#pragma once
#include <omniscript/engine/Backends/LLVM/LLVMExternalFunctionResolver.h>

// Windows-specific resolvers
class WindowsAPIResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, 
                          llvm::FunctionType* funcType, LinkDependencies& deps) override;
    
    static bool isWindowsAPIFunction(const std::string& name);
    
private:
    bool isKernel32Function(const std::string& name);
    bool isUser32Function(const std::string& name);
    bool isGdi32Function(const std::string& name);
    std::string getRequiredDLL(const std::string& name);
};