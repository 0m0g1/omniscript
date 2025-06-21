#include <omniscript/engine/Backends/LLVM/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolver.h>

// WebAssemblyResolver Implementation
llvm::Function* WebAssemblyResolver::resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) {
    if (PlatformInfo::getCurrentPlatform() != PlatformInfo::WebAssembly) {
        return nullptr;
    }
    
    if (isWASIFunction(name) || isEmscriptenFunction(name) || isWebAPIFunction(name)) {
        auto func = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            name,
            generator.getCurrentModule()
        );
        
        applyWasmAttributes(func, name);
        return func;
    }
    
    return nullptr;
}

bool WebAssemblyResolver::isWASIFunction(const std::string& name) {
    return name.find("__wasi_") == 0;
}

bool WebAssemblyResolver::isEmscriptenFunction(const std::string& name) {
    return name.find("emscripten_") == 0 || name.find("_emscripten_") != std::string::npos;
}

bool WebAssemblyResolver::isWebAPIFunction(const std::string& name) {
    return name.find("console_") == 0 || name.find("fetch_") == 0;
}

void WebAssemblyResolver::applyWasmAttributes(llvm::Function* func, const std::string& name) {
    func->setCallingConv(llvm::CallingConv::C);
    
    // WASM functions are typically marked as external
    if (isWASIFunction(name) || isEmscriptenFunction(name)) {
        func->addFnAttr(llvm::Attribute::NoUnwind);
    }
}