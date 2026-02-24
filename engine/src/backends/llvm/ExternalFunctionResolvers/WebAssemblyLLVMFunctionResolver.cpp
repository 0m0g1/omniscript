#include <omniscript/backends/llvm/IRGenerator.h>
#include <omniscript/backends/llvm/ExternalFunctionResolvers/WebAssemblyLLVMResolver.h>

namespace Omniscript {
llvm::Function* WebAssemblyResolver::resolve(IRGenerator& generator, const std::string& name, 
                                            llvm::FunctionType* funcType, LinkDependencies& deps) {
    if (PlatformInfo::getCurrentPlatform() != PlatformInfo::WebAssembly) {
        return nullptr; // Only resolve for WebAssembly
    }
    
    if (!isWASIFunction(name) && !isEmscriptenFunction(name) && !isWebAPIFunction(name)) {
        return nullptr; // Unknown WebAssembly function
    }
    
    // Create the function
    llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, generator.getModule());
    
    // Apply WebAssembly-specific attributes
    applyWasmAttributes(func, name);
    
    // WASI and Emscripten functions typically don't need explicit linking
    // They're resolved by the runtime or during WASM linking
    
    return func;
}

bool WebAssemblyResolver::isWASIFunction(const std::string& name) {
    static const std::unordered_set<std::string> wasiFunctions = {
        "fd_read", "fd_write", "fd_close", "fd_seek", "path_open",
        "environ_get", "environ_sizes_get", "args_get", "args_sizes_get",
        "clock_time_get", "random_get", "poll_oneoff", "proc_exit"
    };
    return wasiFunctions.find(name) != wasiFunctions.end();
}

bool WebAssemblyResolver::isEmscriptenFunction(const std::string& name) {
    return name.find("emscripten_") == 0 || name.find("_emscripten_") == 0;
}

bool WebAssemblyResolver::isWebAPIFunction(const std::string& name) {
    static const std::unordered_set<std::string> webApiFunctions = {
        "console_log", "fetch", "setTimeout", "setInterval", "clearTimeout", "clearInterval"
    };
    return webApiFunctions.find(name) != webApiFunctions.end();
}

void WebAssemblyResolver::applyWasmAttributes(llvm::Function* func, const std::string& name) {
    // Set WebAssembly calling convention
    func->setCallingConv(llvm::CallingConv::C);
    
    // Add import attributes for WASI functions
    if (isWASIFunction(name)) {
        func->addFnAttr("wasm-import-module", "wasi_snapshot_preview1");
        func->addFnAttr("wasm-import-name", name);
    } else if (isWebAPIFunction(name)) {
        func->addFnAttr("wasm-import-module", "env");
        func->addFnAttr("wasm-import-name", name);
    }
}

} // namespace Omniscript