#pragma once
#include <omniscript/engine/Backends/LLVM/LLVMExternalFunctionResolver.h>

// Generic static library resolver
class StaticLibraryResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, 
                          llvm::FunctionType* funcType, LinkDependencies& deps) override;
};