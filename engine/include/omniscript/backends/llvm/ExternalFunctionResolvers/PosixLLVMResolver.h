#pragma once
#include <omniscript/backends/llvm/ExternalFunctionResolvers/PosixLLVMResolver.h>

// POSIX/Unix resolver (Linux, macOS, BSD)
namespace Omniscript {
class PosixResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, 
                          llvm::FunctionType* funcType, LinkDependencies& deps) override;
    
private:
    bool isPthreadFunction(const std::string& name);
    bool isSocketFunction(const std::string& name);
    bool isMathFunction(const std::string& name);
    std::vector<std::string> getRequiredLibraries(const std::string& name);
};

} // namespace Omniscript
