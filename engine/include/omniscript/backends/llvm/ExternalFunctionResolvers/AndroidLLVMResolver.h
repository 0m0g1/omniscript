#pragma once
#include <omniscript/backends/llvm/LLVMExternalFunctionResolver.h>

// Android resolver
namespace Omniscript {
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

} // namespace Omniscript
