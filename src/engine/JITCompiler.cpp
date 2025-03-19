#include <omniscript/engine/JITCompiler.h>

void JITCompiler::execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config &config) {
    console.log("Executing with the JIT Compiler");

    std::vector<std::function<void()>> pendingCalls; // Store top-level function calls

    // Generate IR for all statements
    for (const auto& statement : statements) {
        llvm::Value* ir = statement->codegen(irGen);
        
        // If this statement is a function call, store it for later execution
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

    // Print the generated IR
    console.log("Generated LLVM IR:\n");
    irGen.getModule()->print(llvm::outs(), nullptr);

    // Optimize the module
    irGen.optimizeModule();

    // Wrap LLVMContext in ThreadSafeContext
    llvm::orc::ThreadSafeContext tsContext(irGen.getContext());
    auto module = std::move(irGen.getModule());
    llvm::orc::ThreadSafeModule tsm(std::move(module), tsContext);

    // Add the module to the JIT
    if (auto err = jit->addIRModule(std::move(tsm))) {
        throw std::runtime_error("Failed to add IR module to JIT");
    }

    // Look up `main`
    auto mainSymbol = jit->lookup("main");

    if (mainSymbol) {
        // Execute only main() and ignore top-level function calls
        auto mainFunc = mainSymbol->toPtr<int(*)()>();
        int result = mainFunc();
        console.log("Execution Result: " + std::to_string(result));
    } else {
        // No main function? Execute stored top-level calls
        for (auto& call : pendingCalls) {
            call();
        }
    }
}
