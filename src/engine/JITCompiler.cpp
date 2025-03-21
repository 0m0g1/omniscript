#include <omniscript/engine/JITCompiler.h>

void JITCompiler::execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config &config) {
    console.log("Executing with the JIT Compiler");

    std::vector<std::function<void()>> pendingCalls;

    // Generate IR for all statements
    for (const auto& statement : statements) {
        llvm::Value* ir = statement->codegen(irGen);
        
        if (!ir) continue;

        if (auto* func = llvm::dyn_cast<llvm::Function>(ir)) {
            pendingCalls.push_back([this, func]() {
                auto symbol = jit->lookup(func->getName().str());
                if (symbol) {
                    auto fnPtr = symbol->toPtr<void(*)()>();
                    fnPtr();
                }
            });
        }
    }

    irGen.printIR();
    irGen.printErrors();
    
    irGen.optimizeModule();
    
    llvm::orc::ThreadSafeContext tsContext(irGen.getContext());
    auto module = std::move(irGen.getModule());

    // Step 1: Retrieve function metadata before moving module
    std::string entryPoint;
    llvm::Function* func = nullptr;

    if (!config.entry.empty()) {
        func = module->getFunction(config.entry);
        entryPoint = config.entry;
    } else {
        func = module->getFunction("main");
        if (!func) {
            func = module->getFunction("__top_level__");
            entryPoint = "__top_level__";
        } else {
            entryPoint = "main";
        }
    }

    if (!func) {
        throw std::runtime_error("No valid entry function found (expected 'main' or '__top_level__').");
    }

    // Move module into JIT **before** lookup
    llvm::orc::ThreadSafeModule tsm(std::move(module), tsContext);
    if (auto err = jit->addIRModule(std::move(tsm))) {
        throw std::runtime_error("Failed to add IR module to JIT");
    }

    // Step 2: Lookup function symbol **after module is added**
    auto entrySymbol = jit->lookup(entryPoint);
    if (!entrySymbol) {
        llvm::logAllUnhandledErrors(entrySymbol.takeError(), llvm::errs(), "JIT Lookup Error: ");
        throw std::runtime_error("Failed to find entry symbol: " + entryPoint);
    }

    llvm::Type* returnType = func->getReturnType();

    // Step 3: Execute function based on return type
    if (entryPoint == "__top_level__") {
        if (!returnType->isVoidTy()) {
            throw std::runtime_error("__top_level__ must return void.");
        }
        console.log("Executing top-level code (__top_level__)...");
        auto entryFunc = entrySymbol->toPtr<void(*)()>();
        entryFunc();
        console.log("Execution Completed (void function).");

    } else if (entryPoint == "main") {
        if (!returnType->isIntegerTy(32)) {
            throw std::runtime_error("main must return int.");
        }
        console.log("Executing main function...");
        auto entryFunc = entrySymbol->toPtr<int(*)()>();
        int result = entryFunc();
        console.log("Execution Result: " + std::to_string(result));

    } else { // Custom entry function
        console.log("Executing custom entry function: " + entryPoint + "...");

        if (returnType->isIntegerTy(32)) {
            auto entryFunc = entrySymbol->toPtr<int(*)()>();
            int result = entryFunc();
            console.log("Execution Result: " + std::to_string(result));
        } else if (returnType->isVoidTy()) {
            auto entryFunc = entrySymbol->toPtr<void(*)()>();
            entryFunc();
            console.log("Execution Completed (void function).");
        } else {
            throw std::runtime_error("Unsupported return type for custom entry function.");
        }
    }

    // Step 4: Run pending calls **only if `main` is absent**
    if (config.entry.empty() && !jit->lookup("main")) {
        console.log("Executing pending calls...");
        for (auto& call : pendingCalls) {
            call();
        }
    }
}
