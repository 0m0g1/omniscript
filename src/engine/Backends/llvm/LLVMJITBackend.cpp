#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Backends/llvm/LLVMJITBackend.h>

void LLVMJITBackend::initialize() {
    // This can be expanded to include any JIT-specific initialization logic
    // If needed, for example, adding dynamic library search generators:
    // jit->getMainJITDylib().addGenerator(
    //     llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
    //     jit->getDataLayout().getGlobalPrefix()
    //     ))
    // );
}

void LLVMJITBackend::execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config& config) {
    DEBUG_LOG();
    DEBUG_LOG("Executing with LLVM JIT Backend");
    DEBUG_LOG("===============================");

    scope->setName(config.filePath);
    
    irGen = std::make_shared<IRGenerator>(config.filePath);

    std::vector<std::function<void()>> pendingCalls;

    // Generate IR for all statements
    DEBUG_LOG("Evaluating statements");
    DEBUG_LOG("====================");
    
    for (const auto& statement : statements) {
        DEBUG_LOG();
        DEBUG_LOG("1. Evaluating " + statement->toString());
        Omniscript::setPosition(statement->getPosition());
        std::shared_ptr<Omniscript::Expression> result = statement->express(scope);
        if (!result) continue;

        DEBUG_LOG();
        DEBUG_LOG("2. Generating LLVM IR for '" + result->toString() + "'.");
        
        // Generate LLVM IR for each statement
        llvm::Value* ir = irGen->codegen(result, scope);
        if (!ir) continue;

        // If the IR generated is a function, add it to the list of pending calls
        if (auto* func = llvm::dyn_cast<llvm::Function>(ir)) {
            pendingCalls.push_back([this, func]() {
                auto symbol = jit->lookup(func->getName().str());
                if (symbol) {
                    auto fnPtr = symbol->toPtr<void(*)()>();
                    fnPtr();  // Execute the function via JIT
                }
            });
        }

        DEBUG_LOG("Done Generating IR for " + result->toString() + "'.");
        DEBUG_LOG();
        DEBUG_LOG();
    }
    DEBUG_LOG();
    DEBUG_LOG("Done evaluating statements");
    DEBUG_LOG("==========================");
    DEBUG_LOG();

    // Finalize global initializers, print the IR, and optimize the module
    irGen->finalizeGlobalInitializers();
    irGen->finalize();

    irGen->optimizeModule(config.optimizationLevel);
    
    if (config.logFinalCode) {
        DEBUG_LOG();
        irGen->printIR();
        DEBUG_LOG();
        irGen->printErrors();
        DEBUG_LOG();
    }

    llvm::orc::ThreadSafeContext tsContext(irGen->getContext());
    std::unique_ptr<llvm::Module> module = std::move(irGen->getModule());
    if (config.logAsm) {
        DEBUG_LOG();
        irGen->printAssembly(module.get());
        DEBUG_LOG();
    }
    
    // Initialize globals
    if (auto startupSym = jit->lookup("__startup__")) {
        auto startupFn = startupSym->toPtr<void(*)()>();
        startupFn();  // Initialize globals first
    }
    
    // Retrieve the entry point function
    std::string entryPoint;
    llvm::Function* func = nullptr;
    
    if (!config.entry.empty()) {
        func = module->getFunction(config.entry);
        entryPoint = config.entry;
    } else {
        func = module->getFunction("__main");
        if (!func) {
            func = module->getFunction("__top_level__");
            entryPoint = "__top_level__";
        } else {
            entryPoint = "__main";
        }
    }
    
    if (!func) {
        console.error("No valid entry function found (expected '__main' or '__top_level__').");
    }
    
    // Add the module to the JIT
    llvm::orc::ThreadSafeModule tsm(std::move(module), tsContext);
    if (auto err = jit->addIRModule(std::move(tsm))) {
        console.error("Failed to add IR module to JIT");
    }
    
    // Look up the entry point function
    auto entrySymbol = jit->lookup(entryPoint);
    if (!entrySymbol) {
        llvm::logAllUnhandledErrors(entrySymbol.takeError(), llvm::errs(), "JIT Lookup Error: ");
        console.error("Failed to find entry symbol: " + entryPoint);
    }
    
    llvm::Type* returnType = func->getReturnType();
    
    // Execute the entry function based on its return type
    if (entryPoint == "__top_level__") {
        if (!returnType->isVoidTy()) {
            console.error("__top_level__ must return void.");
        }
        DEBUG_LOG("Executing top-level code (__top_level__)...");
        auto entryFunc = entrySymbol->toPtr<void(*)()>();
        entryFunc();
        DEBUG_LOG("Execution Completed (void function).");
        
    } else if (entryPoint == "__main") {
        if (!returnType->isIntegerTy(32)) {
            console.error("__main must return and int 32.");
        }
        DEBUG_LOG("Executing __main function...");
        auto entryFunc = entrySymbol->toPtr<int(*)()>();
        int result = entryFunc();
        DEBUG_LOG("Execution Result: " + std::to_string(result));
        
    } else { // Custom entry function
        DEBUG_LOG("Executing custom entry function: " + entryPoint + "...");
        
        if (returnType->isIntegerTy(32)) {
            auto entryFunc = entrySymbol->toPtr<int(*)()>();
            int result = entryFunc();
            DEBUG_LOG("Execution Result: " + std::to_string(result));
        } else if (returnType->isVoidTy()) {
            auto entryFunc = entrySymbol->toPtr<void(*)()>();
            entryFunc();
            DEBUG_LOG("Execution Completed (void function).");
        } else {
            console.error("Unsupported return type for custom entry function.");
        }
    }
    
    // Execute any pending calls if needed
    if (config.entry.empty() && !jit->lookup("__main")) {
        DEBUG_LOG("Executing pending calls...");
        for (auto& call : pendingCalls) {
            call();
        }
    }
}