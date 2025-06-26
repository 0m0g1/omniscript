#pragma once
<<<<<<< HEAD:include/omniscript/Backends/LLVM/ExternalFunctionResolvers/DynamicLibraryLLVMResolver.h
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/DynamicLibraryLLVMResolver.h>
=======
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/DynamicLibraryLLVMResolver.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:include/omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/DynamicLibraryLLVMResolver.h

// Generic dynamic library resolver (cross-platform)
class DynamicLibraryResolver : public ExternalFunctionResolver {
public:
    DynamicLibraryResolver(const std::string& libPath);
    llvm::Function* resolve(IRGenerator& generator, const std::string& name, 
                          llvm::FunctionType* funcType, LinkDependencies& deps) override;
    
private:
    std::string libPath_;
    llvm::sys::DynamicLibrary dynLib;
    static std::string normalizePath(const std::string& path);
};