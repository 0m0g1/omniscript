#include <omniscript/engine/JITCompiler.h>

void JITCompiler::execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config &config) {
    console.log("Executing with the JIT Compiler");

    // Generate IR for all statements
    for (const auto& statement : statements) {
        llvm::Value* ir = static_cast<StatementCRTP<decltype(*statement.get())>*>(statement.get())->generateIR(irGen);
        // Execute IR via JIT runtime
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

    // Look up the main function
    auto mainSymbol = jit->lookup("main");
    if (!mainSymbol) {
        throw std::runtime_error("Failed to find main function in JIT");
    }

    // Execute the main function
    auto mainFunc = mainSymbol->toPtr<int(*)()>();
    int result = mainFunc();
    console.log("Execution Result: " + std::to_string(result));
}
