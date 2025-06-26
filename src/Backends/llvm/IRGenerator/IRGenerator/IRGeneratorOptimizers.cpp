#include <omniscript/Backends/LLVM/IRGenerator.h>

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>

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

void IRGenerator::optimizeModule(int level) {
    if (level == -1) {
        DEBUG_LOG("No optimization taking place");
        return;
    }

    if (!Module) {
        console.error("Module is null before optimization");
        return;
    }

    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager cam;
    llvm::ModuleAnalysisManager mam;
    llvm::PassBuilder pb;

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
