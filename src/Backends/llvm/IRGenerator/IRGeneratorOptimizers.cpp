#include <omniscript/Backends/LLVM/IRGenerator.h>

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

namespace Omniscript {
void IRGenerator::setupOptimizationPipeline() {
    DEBUG_LOG("setting up optimization pipeline, (does nothing for now)");
    // This would set up the optimization pipeline based on config
    // Implementation depends on your optimization framework
    // if (configs.diagnostics.logOptimizationRemarks) {
    //     // Enable optimization remarks
    //     Context->setDiagnosticHandlerCallBack([](const llvm::DiagnosticInfo& DI, void* Context) {
    //         if (DI.getKind() == llvm::DK_OptimizationRemark ||
    //             DI.getKind() == llvm::DK_OptimizationRemarkMissed ||
    //             DI.getKind() == llvm::DK_OptimizationRemarkAnalysis) {
    //             std::string msg;
    //             llvm::raw_string_ostream stream(msg);
    //             DI.print(stream);
    //             llvm::outs() << "Optimization: " << msg << "\n";
    //         }
    //     }, nullptr);
    // }
}

// A TargetMachine is built below ONLY to give the optimizer a TargetTransformInfo (vector widths,
// instruction costs, unroll/inline heuristics). Without it PassBuilder falls back to a generic,
// target-less cost model: the loop vectorizer and unroller are very conservative and CPU features
// (AVX2, FMA, AVX-512) are invisible to every IR pass.

void IRGenerator::optimizeModule(int level) {
    if (level == -1) {
        DEBUG_LOG("No optimization taking place");
        return;
    }

    if (!Module) {
        console.reportError(Console::INTERNAL_ERROR, "Module is null before optimization", FileSpan{});
        return;
    }

    // ---- target machine for the cost model --------------------------------------------------
    std::unique_ptr<llvm::TargetMachine> tm;
    {
        std::string triple = Module->getTargetTriple().str();
        if (triple.empty()) {
            triple = llvm::sys::getProcessTriple();
        }

        std::string error;
        const llvm::Target* target = llvm::TargetRegistry::lookupTarget(llvm::Triple(triple), error);
        if (!target) {
            DEBUG_LOG("Optimizer: no target for '" + triple + "' (" + error + "); optimizing without target info");
        } else {
            std::string cpu;
            std::string features;
            if (configs.cpuFeatures == "native" && !configs.isCrossCompilation()) {
                cpu = llvm::sys::getHostCPUName().str();
                for (const auto& feature : llvm::sys::getHostCPUFeatures()) {
                    if (!features.empty()) features += ",";
                    features += (feature.getValue() ? "+" : "-") + feature.getKey().str();
                }
            } else {
                cpu = resolveCPUName(triple);
                features = buildFeatureString(triple);
            }

            llvm::CodeGenOptLevel cgLevel =
                level <= 0 ? llvm::CodeGenOptLevel::None :
                level == 1 ? llvm::CodeGenOptLevel::Less :
                level == 2 ? llvm::CodeGenOptLevel::Default :
                             llvm::CodeGenOptLevel::Aggressive;

            llvm::TargetOptions opts;
            tm.reset(target->createTargetMachine(llvm::Triple(triple), cpu, features, opts,
                                                 std::nullopt, std::nullopt, cgLevel));
            if (tm) {
                DEBUG_LOG("Optimizer target: " + triple + " cpu=" + cpu);
                if (Module->getTargetTriple().str().empty()) {
                    Module->setTargetTriple(tm->getTargetTriple());
                    Module->setDataLayout(tm->createDataLayout());
                }
            } else {
                DEBUG_LOG("Optimizer: createTargetMachine failed; optimizing without target info");
            }
        }
    }

    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager cam;
    llvm::ModuleAnalysisManager mam;

    llvm::PipelineTuningOptions pto;
    pto.LoopVectorization = configs.optimization.enableVectorization && level >= 2;
    pto.SLPVectorization  = configs.optimization.enableVectorization && level >= 2;
    pto.LoopUnrolling     = configs.optimization.enableLoopUnrolling;
    pto.LoopInterleaving  = true;

    llvm::PassBuilder pb(tm.get(), pto);

    pb.registerModuleAnalyses(mam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.registerCGSCCAnalyses(cam);
    pb.crossRegisterProxies(lam, fam, cam, mam);

    llvm::OptimizationLevel optLevel = llvm::OptimizationLevel::O2;
    if (level == 0) optLevel = llvm::OptimizationLevel::O0;
    else if (level == 1) optLevel = llvm::OptimizationLevel::O1;
    else if (level == 2) optLevel = llvm::OptimizationLevel::O2;
    else if (level >= 3) optLevel = llvm::OptimizationLevel::O3;

    DEBUG_LOG("Building optimization pipeline...");
    llvm::ModulePassManager mpm = pb.buildPerModuleDefaultPipeline(optLevel);

    DEBUG_LOG("Running optimization passes...");
    mpm.run(*Module, mam);
    DEBUG_LOG("Optimization complete");
}

}