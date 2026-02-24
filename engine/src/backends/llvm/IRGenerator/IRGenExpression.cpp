#include <omniscript/backends/llvm/IRGenerator.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

#include <stdexcept>
#include <string>
#include <utility>

namespace Omniscript {

IRGenerator::IRGenerator(const Config& config)
    : m_config(config)
    , m_tsc(std::make_unique<llvm::LLVMContext>()) {}

void IRGenerator::initialize() {
    auto& C = *m_tsc.getContext();

    const std::string modName =
        !m_config.filePath.empty() ? m_config.filePath : "omniscript_module";

    m_module  = std::make_unique<llvm::Module>(modName, C);
    m_builder = std::make_unique<llvm::IRBuilder<>>(C);

    if (m_targetMachine) {
        m_module->setTargetTriple(m_targetMachine->getTargetTriple().str());
        m_module->setDataLayout(m_targetMachine->createDataLayout());
    }
}

void IRGenerator::finalize() {
    if (!m_module) return;

    std::string err;
    llvm::raw_string_ostream os(err);
    if (llvm::verifyModule(*m_module, &os)) {
        os.flush();
        llvm::errs() << "[IRGenerator] verifyModule failed:\n" << err << "\n";
        throw std::runtime_error("LLVM IR verification failed");
    }
}

llvm::orc::ThreadSafeModule IRGenerator::takeThreadSafeModule() {
    auto M = std::move(m_module);
    return llvm::orc::ThreadSafeModule(std::move(M), m_tsc);
}

std::unique_ptr<llvm::Module> IRGenerator::takeModule() {
    return std::move(m_module);
}

llvm::LLVMContext& IRGenerator::getContext() {
    return *m_tsc.getContext();
}

llvm::IRBuilder<>& IRGenerator::getBuilder() {
    return *m_builder;
}

void IRGenerator::setTargetMachine(std::shared_ptr<llvm::TargetMachine> tm) {
    m_targetMachine = std::move(tm);
    if (m_module && m_targetMachine) {
        m_module->setTargetTriple(m_targetMachine->getTargetTriple().str());
        m_module->setDataLayout(m_targetMachine->createDataLayout());
    }
}

void IRGenerator::addExternalResolver(const std::string& language,
                                      std::unique_ptr<ExternalFunctionResolver> resolver) {
    if (!resolver) return;
    m_resolvers[language] = std::move(resolver);
}

llvm::Type* IRGenerator::toLLVMType(const Type& /*t*/) {
    // TODO: wire semantic Type -> LLVM type
    return llvm::Type::getInt64Ty(getContext());
}

} // namespace Omniscript