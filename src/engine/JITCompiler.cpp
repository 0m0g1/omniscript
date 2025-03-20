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
    irGen.optimizeModule();

    llvm::orc::ThreadSafeContext tsContext(irGen.getContext());
    auto module = std::move(irGen.getModule());
    llvm::orc::ThreadSafeModule tsm(std::move(module), tsContext);

    if (auto err = jit->addIRModule(std::move(tsm))) {
        throw std::runtime_error("Failed to add IR module to JIT");
    }

    llvm::Expected<llvm::orc::ExecutorAddr> mainSymbol = 
        !config.entry.empty() ? jit->lookup(config.entry) : jit->lookup("main");

    if (!mainSymbol) {
        console.error("The entry point " + config.entry + " was not defined.");
    } else {
        auto mainFunc = mainSymbol->toPtr<int(*)()>();
        int result = mainFunc();
        console.log("Execution Result: " + std::to_string(result));
    }

    for (auto& call : pendingCalls) {
        call();
    }
}
