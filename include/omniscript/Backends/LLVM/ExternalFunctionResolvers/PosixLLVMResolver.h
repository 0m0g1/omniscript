#pragma once
<<<<<<< HEAD:include/omniscript/Backends/LLVM/ExternalFunctionResolvers/PosixLLVMResolver.h
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/PosixLLVMResolver.h>
=======
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/PosixLLVMResolver.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:include/omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/PosixLLVMResolver.h

// POSIX/Unix resolver (Linux, macOS, BSD)
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