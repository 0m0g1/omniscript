#pragma once
<<<<<<< HEAD:include/omniscript/Backends/LLVM/ExternalFunctionResolvers/SmartPlatformLLVMResolver.h
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
=======
#include <omniscript/engine/Backends/LLVM/LLVMExternalFunctionResolver.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:include/omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/SmartPlatformLLVMResolver.h

// Smart resolver that automatically selects the best resolver for the platform
class SmartPlatformResolver : public ExternalFunctionResolver {
public:
    SmartPlatformResolver();
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, 
                          llvm::FunctionType* funcType, LinkDependencies& deps) override;
    
    // Allow manual registration of custom resolvers
    void registerResolver(const std::string& pattern, std::unique_ptr<ExternalFunctionResolver> resolver);
    
private:
    std::vector<std::unique_ptr<ExternalFunctionResolver>> platformResolvers_;
    std::unordered_map<std::string, std::unique_ptr<ExternalFunctionResolver>> customResolvers_;
    
    void initializePlatformResolvers();
    ExternalFunctionResolver* findBestResolver(const std::string& name);
};