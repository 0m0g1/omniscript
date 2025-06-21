#pragma once
#include <omniscript/engine/Backends/LLVM/LLVMExternalFunctionResolver.h>

// macOS/iOS resolver
class DarwinResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, 
                          llvm::FunctionType* funcType, LinkDependencies& deps) override;
    
private:
    bool isFoundationFunction(const std::string& name);
    bool isCoreFoundationFunction(const std::string& name);
    bool isCocoaFunction(const std::string& name);
    std::string getRequiredFramework(const std::string& name);
};