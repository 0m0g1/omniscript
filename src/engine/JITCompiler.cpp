#include <omniscript/engine/JITCompiler.h>

void JITCompiler::execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config &config) {
    // for (const auto& statement : statements) {
    //     llvm::Value* ir = static_cast<StatementCRTP<decltype(*statement.get())>*>(statement.get())->generateIR(irGen);
    //     // Execute IR via JIT runtime
    // }
    std::unique_ptr<llvm::Module> module = irGen.createModule();

    console.log("Generated LLVM IR:\n");
    module->print(llvm::outs(), nullptr);

    optimizeModule(*module);

    auto tsm = llvm::orc::ThreadSafeModule(std::move(module), irGen.getContext());
    if (auto err = jit->addIRModule(std::move(tsm))) {
        throw std::runtime_error("Failed to add IR module to JIT");
    }

    auto mainSymbol = jit->lookup("main");
    if (!mainSymbol) {
        throw std::runtime_error("Failed to find main function in JIT");
    }

    auto mainFunc = (int(*)())mainSymbol->getAddress();
    int result = mainFunc();
    console.log("Execution Result: " + std::to_string(result));
}
