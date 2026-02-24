#pragma once
#include <omniscript/backends/llvm/LLVMExternalFunctionResolver.h>

// macOS/iOS resolver
namespace Omniscript {
class DarwinResolver : public ExternalFunctionResolver {
public:
    DarwinResolver(); // Default constructor
    DarwinResolver(const std::string& libraryPath);
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, 
                          llvm::FunctionType* funcType, LinkDependencies& deps) override;
    
private:
    std::string specifiedLibraryPath;
    bool isFoundationFunction(const std::string& name);
    bool isCoreFoundationFunction(const std::string& name);
    bool isCocoaFunction(const std::string& name);
    std::string getRequiredFramework(const std::string& name);
};

} // namespace Omniscript
