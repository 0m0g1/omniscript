#include <omniscript/engine/Backends/LLVM/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/StaticLibraryLLVMResolver.h>

// StaticLibraryResolver Implementation
llvm::Function* StaticLibraryResolver::resolve(IRGenerator& generator, const std::string& name, 
                                             llvm::FunctionType* funcType, LinkDependencies& deps) {
    // For static libraries, we typically just create the function declaration
    // and let the linker resolve it later. The actual symbol resolution happens
    // during the linking phase with the .a/.lib files.
    
    // Create the function
    llvm::Function* func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, name, generator.getCurrentModule()
    );
    
    // Note: Static library dependencies would be added by the caller
    // since this resolver doesn't know which specific static library to use
    
    return func;
}