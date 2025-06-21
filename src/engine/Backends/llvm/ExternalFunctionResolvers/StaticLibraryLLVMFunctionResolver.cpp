
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/StaticLibraryLLVMResolver.h>

llvm::Function* StaticLibraryResolver::resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) {
    // For static libraries, we just create the function declaration
    // The actual linking will be handled by the linker
    return llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        name,
        generator.getCurrentModule()
    );
}