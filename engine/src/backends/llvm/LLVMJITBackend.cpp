// engine/src/backends/llvm/LLVMJITBackend.cpp

#include <omniscript/backends/llvm/LLVMJITBackend.h>
#include <omniscript/backends/llvm/IRGenerator.h>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>

#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h> // LLVM 20: getHostCPUName lives here in your setup

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>

namespace Omniscript {

LLVMJITBackend::LLVMJITBackend() = default;

void LLVMJITBackend::initialize(const Config& config) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    std::string validationError;
    if (!config.validate(validationError)) {
        throw std::runtime_error("Configuration validation failed: " + validationError);
    }
}

void LLVMJITBackend::setupJITEngine(const Config& config) {
    if (config.isCrossCompilation()) {
        throw std::runtime_error("JIT not supported for cross-compilation targets");
    }

    auto jtmbExpected = createTargetMachineBuilder(config);
    if (!jtmbExpected) {
        throw std::runtime_error(
            "Failed to create JITTargetMachineBuilder: " +
            llvm::toString(jtmbExpected.takeError())
        );
    }

    auto jitOrErr = llvm::orc::LLJITBuilder()
        .setJITTargetMachineBuilder(std::move(*jtmbExpected))
        .create();

    if (!jitOrErr) {
        throw std::runtime_error("Failed to create LLJIT: " + llvm::toString(jitOrErr.takeError()));
    }

    m_jit = std::move(*jitOrErr);

    auto gen = llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
        m_jit->getDataLayout().getGlobalPrefix()
    );
    if (!gen) {
        throw std::runtime_error(
            "Failed to create host process resolver: " +
            llvm::toString(gen.takeError())
        );
    }
    m_jit->getMainJITDylib().addGenerator(std::move(*gen));

    configureJITOptions(config);
}

llvm::Expected<llvm::orc::JITTargetMachineBuilder>
LLVMJITBackend::createTargetMachineBuilder(const Config& config) {
    auto jtmbExpected = llvm::orc::JITTargetMachineBuilder::detectHost();
    if (!jtmbExpected) return jtmbExpected.takeError();

    auto builder = std::move(*jtmbExpected);

    llvm::TargetOptions opt;
    opt.UnsafeFPMath          = config.optimization.fastMath;
    opt.NoInfsFPMath          = config.optimization.fastMath;
    opt.NoNaNsFPMath          = config.optimization.fastMath;
    opt.NoSignedZerosFPMath   = config.optimization.fastMath;
    opt.GuaranteedTailCallOpt = config.optimization.enableTailCallOptimization;
    builder.setOptions(opt);

    builder.setCPU(llvm::sys::getHostCPUName().str());
    return std::move(builder);
}

void LLVMJITBackend::configureJITOptions(const Config&) {
    // hooks later
}

void LLVMJITBackend::execute(const Program& program, const Config& config) {
    m_compilationStartTime = std::chrono::system_clock::now();

    if (!m_jit) setupJITEngine(config);

    if (!m_irGen) m_irGen = std::make_shared<IRGenerator>(config);
    m_irGen->initialize();
    m_irGen->codegenProgram(program);
    m_irGen->finalize();

    // --- This is how you "generate the IR" in-memory (see below for printing/writing) ---

    // Add module to JIT
    llvm::orc::ThreadSafeModule tsm = m_irGen->takeThreadSafeModule();
    if (auto err = m_jit->addIRModule(std::move(tsm))) {
        throw std::runtime_error("Failed to add module to JIT: " + llvm::toString(std::move(err)));
    }

    const std::string entry = !config.entry.empty() ? config.entry : "main";

    // LLVM 20: lookup returns Expected<ExecutorAddr>
    auto sym = m_jit->lookup(entry);
    if (!sym) {
        throw std::runtime_error(
            "Entry function '" + entry + "' not found: " +
            llvm::toString(sym.takeError())
        );
    }

    using EntryFn = int (*)();

    llvm::orc::ExecutorAddr addr = *sym;      // ✅ LLVM 20
    EntryFn fn = addr.toPtr<EntryFn>();       // ✅ convert to function pointer

    if (!fn) {
        throw std::runtime_error("Failed to get function pointer for entry: " + entry);
    }

    (void)fn();
}

void LLVMJITBackend::cleanup() {
    m_irGen.reset();
    m_jit.reset();
}

} // namespace Omniscript