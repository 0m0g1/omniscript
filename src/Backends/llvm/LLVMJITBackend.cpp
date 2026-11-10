// LLVMJITBackend.cpp
#include <cstdlib>
#include <system_error>
#include <llvm/Support/raw_ostream.h>
#include <omniscript/Backends/llvm/LLVMJITBackend.h>
#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/Program.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <thread>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <type_traits>
#if defined(_MSC_VER)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace Omniscript {

#if defined(_MSC_VER)
namespace {
// Runs fn(ctx) and converts a hardware exception (access violation etc.) into
// a return code. Must not contain C++ objects with destructors (MSVC rule).
unsigned long runWithSEH(void (*fn)(void*), void* ctx) {
    __try {
        fn(ctx);
        return 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return static_cast<unsigned long>(GetExceptionCode());
    }
}
} // namespace
#endif

LLVMJITBackend::LLVMJITBackend() {
    scope = std::make_shared<SymbolTable<std::shared_ptr<Expression>, std::shared_ptr<Type>>>();
    initialize();
}

void LLVMJITBackend::initialize() {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
    DEBUG_LOG("LLVMJITBackend initialized LLVM target registry");
}

void LLVMJITBackend::setupJITEngine(const Config& config) {
    DEBUG_LOG("Setting up the JIT engine");
    if (config.isCrossCompilation()) {
        logError(config, "JIT compilation is not supported for cross-compilation targets");
        throw std::runtime_error("JIT compilation not supported for cross-compilation");
    }

    auto jtmbExpected = createTargetMachineBuilder(config);
    if (!jtmbExpected) {
        logError(config, "Failed to create JIT target machine builder: " + llvm::toString(jtmbExpected.takeError()));
        throw std::runtime_error("Failed to create JIT target machine builder");
    }

    auto jtmb = std::move(*jtmbExpected);
    auto jitOrError = llvm::orc::LLJITBuilder()
        .setJITTargetMachineBuilder(std::move(jtmb))
        .create();

    if (!jitOrError) {
        logError(config, "Failed to create LLJIT: " + llvm::toString(jitOrError.takeError()));
        throw std::runtime_error("Failed to create LLJIT");
    }

    jit = std::move(jitOrError.get());
    configureJITOptions(config);
    DEBUG_LOG("JIT engine initialized with target: " + config.getTargetSummary());
}

llvm::Expected<llvm::orc::JITTargetMachineBuilder> LLVMJITBackend::createTargetMachineBuilder(const Config& config) {
    auto jtmbExpected = llvm::orc::JITTargetMachineBuilder::detectHost();
    if (!jtmbExpected) {
        return jtmbExpected.takeError();
    }

    auto builder = std::move(*jtmbExpected);

    llvm::TargetOptions opt;
    // opt.UnsafeFPMath: removed in newer LLVM (use function attributes / fast-math flags instead)
    // opt.NoInfsFPMath: removed in newer LLVM (use function attributes / fast-math flags instead)
    // opt.NoNaNsFPMath: removed in newer LLVM (use function attributes / fast-math flags instead)
    // opt.NoSignedZerosFPMath: removed in newer LLVM (use function attributes / fast-math flags instead)
    opt.GuaranteedTailCallOpt = config.optimization.enableTailCallOptimization;
    opt.ThreadModel = (config.runtime.enableParallelGC || config.runtime.gcThreads > 1) ?
        llvm::ThreadModel::POSIX : llvm::ThreadModel::Single;

    if (config.security.enableControlFlowIntegrity) {
        opt.EnableMachineOutliner = true;
    }

    builder.setOptions(opt);

    std::string targetTriple = builder.getTargetTriple().str();
    std::string resolvedCPU = irGen ? irGen->resolveCPUName(targetTriple) : llvm::sys::getHostCPUName().str();
    builder.setCPU(resolvedCPU);

    std::string features = irGen ? irGen->buildFeatureString(targetTriple) : "";
    if (!features.empty()) {
        std::vector<std::string> featureVec;
        std::stringstream ss(features);
        std::string feature;
        while (std::getline(ss, feature, ',')) {
            if (!feature.empty()) {
                featureVec.push_back(feature);
            }
        }
        if (!featureVec.empty()) {
            builder.addFeatures(featureVec);
        }
    }

    return std::move(builder);
}

void LLVMJITBackend::configureJITOptions(const Config& config) {
    // Configure JIT options as needed (e.g., lazy compilation, debugging)
    DEBUG_LOG("JIT options configured");
}

void LLVMJITBackend::setupExternalResolvers(const Config& config) {
    auto targetOS = config.resolveTargetOS();
    DEBUG_LOG("Setting up JIT external resolvers for: " + config.getOSName());

    auto hostResolver = createHostProcessResolver(config);
    if (hostResolver) {
        jit->getMainJITDylib().addGenerator(std::move(hostResolver));
    }

    if (!irGen) {
        irGen = std::make_shared<IRGenerator>(config);
    }

    irGen->addExternalResolver("C", std::make_unique<CStdLibResolver>());

    // for (const auto& lib : config.aot.libraries) {
    //     irGen->registerExternalLibrary(lib, config.aot.libraryPaths);
    // }
    // for (const auto& framework : config.aot.frameworks) {
    //     irGen->registerFramework(framework, config.aot.frameworkPaths);
    // }
    // for (const auto& [key, value] : config.environmentVariables) {
    //     irGen->setEnvironmentVariable(key, value);
    // }

    switch (targetOS) {
        case TargetOS::Windows:
            irGen->addExternalResolver("kernel32", std::make_unique<WindowsAPIResolver>("kernel32"));
            irGen->addExternalResolver("user32", std::make_unique<WindowsAPIResolver>("user32"));
            irGen->addExternalResolver("msvcrt", std::make_unique<WindowsAPIResolver>("msvcrt"));
            break;
        case TargetOS::Linux:
        case TargetOS::FreeBSD:
            irGen->addExternalResolver("libc", std::make_unique<PosixResolver>());
            irGen->addExternalResolver("libm", std::make_unique<PosixResolver>());
            irGen->addExternalResolver("libdl", std::make_unique<PosixResolver>());
            irGen->addExternalResolver("libpthread", std::make_unique<PosixResolver>());
            break;
        case TargetOS::MacOS:
        case TargetOS::iOS:
            irGen->addExternalResolver("libSystem", std::make_unique<DarwinResolver>());
            break;
        case TargetOS::Android:
            irGen->addExternalResolver("libc", std::make_unique<AndroidResolver>());
            irGen->addExternalResolver("libm", std::make_unique<AndroidResolver>());
            break;
        default:
            DEBUG_LOG("Using default host process resolver only");
            break;
    }

    registerRuntimeSymbols(config);
    DEBUG_LOG("JIT external resolvers setup completed");
}

std::unique_ptr<llvm::orc::DefinitionGenerator> LLVMJITBackend::createHostProcessResolver(const Config& config) {
    auto resolver = llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
        jit->getDataLayout().getGlobalPrefix());
    if (!resolver) {
        logError(config, "Failed to create host process resolver: " + llvm::toString(resolver.takeError()));
        return nullptr;
    }
    return std::move(resolver.get());
}

void LLVMJITBackend::registerRuntimeSymbols(const Config& config) {
    auto& mainDylib = jit->getMainJITDylib();
    // Example: Register runtime symbols (implement actual functions as needed)
    // mainDylib.define(absoluteSymbol("gc_allocate", ...));
    DEBUG_LOG("Registered runtime symbols");
}

void LLVMJITBackend::execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config& config) {
    compilationStartTime = std::chrono::system_clock::now();
    initializeProfiler(config);
    executePluginCallbacks(config, "pre-execute");

    std::string validationError;
    if (!config.validate(validationError)) {
        logError(config, "Configuration validation failed: " + validationError);
        throw std::runtime_error("Configuration validation failed");
    }

    if (config.diagnostics.verbose) {
        config.printSummary();
    }

    if (config.incremental.enabled && isFileUpToDate(config.mainSourceFile.empty() ? config.filePath : config.mainSourceFile)) {
        DEBUG_LOG("Skipping JIT compilation: Source file up to date");
        executePluginCallbacks(config, "post-execute");
        finalizeProfiler(config);
        return;
    }

    scope->setName(config.mainSourceFile.empty() ? config.filePath : config.mainSourceFile);
    if (!irGen) {
        irGen = std::make_shared<IRGenerator>(config);
    }

    setupJITEngine(config);
    setupExternalResolvers(config);

    // if (config.modules.enableModules) {
    //     for (const auto& modPath : config.modules.modulePaths) {
    //         irGen->loadModule(modPath, config.modules.moduleCachePath);
    //     }
    // }

    // Runs a compiler phase; a crash inside it becomes a reported error that names the phase.
    auto guard = [&](const std::string& phase, auto&& body) {
#if defined(_MSC_VER)
        using Body = std::remove_reference_t<decltype(body)>;
        void (*thunk)(void*) = [](void* p) { (*static_cast<Body*>(p))(); };
        unsigned long code = runWithSEH(thunk, &body);
        if (code != 0) {
            char hex[16];
            std::snprintf(hex, sizeof hex, "0x%08lX", code);
            std::string msg = std::string("Compiler crashed (") + hex + ") during: " + phase +
                ". A previous error probably left a null value that was used here.";
            logError(config, msg);
            console.reportError(Console::ErrorType::RUNTIME_ERROR, msg,
                "Run with --debug and check the last DEBUG line before this message.");
            throw std::runtime_error("Compiler crashed during: " + phase);
        }
#else
        (void)phase;
        body();
#endif
    };

    // Refuses to continue once any error has been reported.
    auto failIfErrors = [&](const std::string& phase) {
        auto n = console.getErrorCount();
        if (n > 0) {
            std::string msg = "Not executing: " + std::to_string(n) + " error(s) reported during " + phase;
            logError(config, msg);
            throw std::runtime_error(msg);
        }
    };

    size_t errorCount = 0;
    for (const auto& statement : statements) {
        
        checkTimeLimit(config);
        
        if (config.maxMemoryUsage != 0 && peakMemoryUsage > config.maxMemoryUsage) {
            std::string message = Console::formatString(
                "Compilation stopped because the program used %d MB of memory, exceeding your limit of %d MB.",
                peakMemoryUsage / (1024 * 1024), config.maxMemoryUsage / (1024 * 1024)
            );
            std::string suggestion = "To fix this:\n"
                                    "1. Increase the memory limit with `--heap-size [size]MB` (e.g., `--heap-size 1024MB`).\n"
                                    "2. Simplify your code by reducing large arrays or structs.\n"
                                    "3. Check for infinite loops or recursive functions that might consume too much memory.\n"
                                    "See compiler options at: https://github.com/0m0g1/omniscript/blob/main/docs/Compiler.md";
            console.reportError(Console::ErrorType::RUNTIME_ERROR, message, suggestion);
            throw std::runtime_error("Compilation memory limit exceeded");
        }

        DEBUG_LOG("Evaluating " + statement->toString());
        setSpan(statement->getSpan());
        std::decay_t<decltype(statement->express(scope))> expr;
        guard("evaluating: " + statement->toString(), [&] { expr = statement->express(scope); });
        if (!expr) {
            if (config.errorHandling.mode == ErrorRecoveryMode::StopOnFirst) {
                logError(config, "Expression evaluation failed for statement: " + statement->toString());
                throw std::runtime_error("Expression evaluation failed");
            }
            if (++errorCount >= config.errorHandling.maxErrorCount) {
                logError(config, "Maximum error count reached");
                throw std::runtime_error("Too many errors");
            }
            continue;
        }

        DEBUG_LOG("Generating LLVM IR for: " + expr->toString());
        guard("IR generation for: " + expr->toString(), [&] { irGen->codegen(expr, scope); });
        trackMemoryUsage(config);
    }

    guard("compiling function bodies", [&] { irGen->compileAllFunctionBodies(scope); });
    guard("finalizing global initializers", [&] { irGen->finalizeGlobalInitializers(); });
    guard("finalizing the module", [&] { irGen->finalize(); });

    if (config.diagnostics.logFinalCode) {
        console.log("========= JIT LLVM IR =========");
        irGen->printIR();
    }

    // Stop before verifying: once errors were reported the IR is incomplete, and the LLVM
    // verifier can crash on it instead of reporting a problem.
    failIfErrors("IR generation");

    // Do not hand a broken module to the JIT.
    if (auto verifyTarget = irGen->getCurrentModule()) {
        // Set OMNISCRIPT_DUMP_IR=path\to\file.ll to write the module out before it is verified.
        // A crash while verifying then leaves the IR behind so the bad function can be found.
        if (const char* dumpPath = std::getenv("OMNISCRIPT_DUMP_IR")) {
            std::error_code dumpEc;
            llvm::raw_fd_ostream dumpOut(dumpPath, dumpEc);
            if (!dumpEc) {
                verifyTarget->print(dumpOut, nullptr);
                dumpOut.flush();
            }
        }
        // Set OMNISCRIPT_VERIFY_TRACE=1 to verify one function at a time and print each name first.
        // If the verifier crashes, the last "[verify]" line names the function with the bad IR.
        if (std::getenv("OMNISCRIPT_VERIFY_TRACE")) {
            for (llvm::Function& fn : *verifyTarget) {
                if (fn.isDeclaration()) continue;
                llvm::errs() << "[verify] " << fn.getName() << "\n";
                llvm::errs().flush();
                llvm::verifyFunction(fn, &llvm::errs());
            }
            llvm::errs() << "[verify] all functions checked\n";
        }
        std::string verifyLog;
        llvm::raw_string_ostream verifyOut(verifyLog);
        if (llvm::verifyModule(*verifyTarget, &verifyOut)) {
            verifyOut.flush();
            console.reportError(Console::ErrorType::RUNTIME_ERROR,
                "LLVM module verification failed:\n" + verifyLog,
                "The generated IR is invalid. This is a compiler bug, not a problem in your program.");
            throw std::runtime_error("LLVM module verification failed");
        }
    }

    if (config.optimization.level > 0) {
        // for (const auto& [pass, options] : config.optimization.customPasses) {
        //     irGen->addCustomPass(pass, options);
        // }
        irGen->optimizeModule(config.optimization.level);
        if (config.diagnostics.logFinalCode) {
            console.log("========= Optimized JIT LLVM IR =========");
            irGen->printIR();
        }
    }

    std::string entryPoint;
    if (!config.entry.empty()) {
        entryPoint = config.entry;
        DEBUG_LOG("Using configured entry point: " + entryPoint);
    } else if (hasMainFunction()) {
        entryPoint = "__main";
        DEBUG_LOG("Found main function with correct signature");
    } else {
        entryPoint = "__top_level__";
        DEBUG_LOG("Using default entry point: __top_level__");
    }

    // Names of external declarations, captured before the module is handed to the JIT.
    std::vector<std::string> externalDecls;
    if (auto* m = irGen->getCurrentModule()) {
        for (auto& f : *m)
            if (f.isDeclaration() && !f.isIntrinsic() && f.hasExternalLinkage())
                externalDecls.push_back(f.getName().str());
    }

    auto module = irGen->getModule();
    auto tsm = llvm::orc::ThreadSafeModule(std::move(module), irGen->getContext());
    auto err = jit->addIRModule(std::move(tsm));
    if (err) {
        logError(config, "Failed to add module to JIT: " + llvm::toString(std::move(err)));
        throw std::runtime_error("Failed to add module to JIT");
    }

    // Pre-flight: every external symbol must resolve before we run anything.
    {
        std::string missing;
        size_t missingCount = 0;
        for (const auto& n : externalDecls) {
            auto s = jit->lookup(n);
            if (s) continue;
            llvm::consumeError(s.takeError());
            if (missingCount++) missing += ", ";
            missing += "'" + n + "'";
        }
        if (missingCount) {
            std::string msg = "Not executing: " + std::to_string(missingCount) +
                " unresolved external symbol(s): " + missing;
            logError(config, msg);
            throw std::runtime_error(msg);
        }
    }

    if (config.isHybridMode() && config.hybrid.enableDynamicFallback) {
        std::filesystem::path aotOutput = config.hybrid.aotOutputPath.empty() ?
            std::filesystem::path(config.outputPath).stem().string() + "_hybrid.o" : config.hybrid.aotOutputPath;
        auto aotModule = irGen->cloneModule();
        emitAOTOutput(std::move(aotModule), aotOutput.string(), config);
        DEBUG_LOG("Hybrid AOT output emitted to: " + aotOutput.string());
    }

    executeFunction(entryPoint, config);
    updateFileCache(config);
    executePluginCallbacks(config, "post-execute");
    finalizeProfiler(config);
}

void LLVMJITBackend::initializeProfiler(const Config& config) {
    if (config.profiler.type == ProfilerType::None || profilerInitialized) {
        return;
    }
    switch (config.profiler.type) {
        case ProfilerType::GProf:
#ifdef _WIN32
            _putenv_s("GMON_OUT_PREFIX", config.profiler.outputPath.c_str());
#else
            setenv("GMON_OUT_PREFIX", config.profiler.outputPath.c_str(), 1);
#endif
            break;
        case ProfilerType::Perf:
#ifndef _WIN32
            if (config.profiler.enableSampling) {
                std::string cmd = "perf record -F " + std::to_string(config.profiler.samplingFrequency) + 
                                  " -o \"" + config.profiler.outputPath + "\" &";
                std::system(cmd.c_str());
            }
#endif
            break;
        case ProfilerType::VTune:
        case ProfilerType::Custom:
            // Placeholder for custom profiler initialization
            break;
        default:
            break;
    }
    profilerInitialized = true;
    DEBUG_LOG("Profiler initialized: " + config.profilerTypeToString(config.profiler.type));
}

void LLVMJITBackend::finalizeProfiler(const Config& config) {
    if (config.profiler.type == ProfilerType::None || !profilerInitialized) {
        return;
    }
    switch (config.profiler.type) {
        case ProfilerType::Perf:
#ifndef _WIN32
            std::system("pkill -SIGINT perf");
#endif
            break;
        default:
            break;
    }
    profilerInitialized = false;
    DEBUG_LOG("Profiler finalized");
}

bool LLVMJITBackend::hasMainFunction() {
    auto module = irGen->getCurrentModule();
    if (!module) return false;

    auto isMainValid = [](llvm::Function* f) {
        if (!f || f->isDeclaration()) return false;
        auto* retType = f->getReturnType();
        bool validReturn = retType->isVoidTy() || retType->isIntegerTy(32);
        if (!validReturn) return false;

        unsigned params = f->getFunctionType()->getNumParams();
        switch (params) {
            case 0: return true;
            case 2: return f->getFunctionType()->getParamType(0)->isIntegerTy(32) &&
                           f->getFunctionType()->getParamType(1)->isPointerTy();
            case 3: return f->getFunctionType()->getParamType(0)->isIntegerTy(32) &&
                           f->getFunctionType()->getParamType(1)->isPointerTy() &&
                           f->getFunctionType()->getParamType(2)->isPointerTy();
            default: return false;
        }
    };

    const char* mainNames[] = {"main", "__main", "_main"};
    for (auto name : mainNames) {
        if (auto* func = module->getFunction(name)) {
            if (isMainValid(func)) {
                for (auto& BB : *func) {
                    for (auto& I : BB) {
                        if (auto* call = llvm::dyn_cast<llvm::CallInst>(&I)) {
                            if (call->getCalledFunction() == func) {
                                logError(Config(), "Recursive main function detected");
                                return false;
                            }
                        }
                    }
                }
                return true;
            }
        }
    }
    return false;
}

void LLVMJITBackend::executeFunction(const std::string& functionName, const Config& config) {
    auto symbol = jit->lookup(functionName);
    if (!symbol) {
        logError(config, "Function '" + functionName + "' not found: " + llvm::toString(symbol.takeError()));
        throw std::runtime_error("Function not found");
    }

    auto fnAddr = symbol.get();
    auto fn = fnAddr.toPtr<int()>();
    if (!fn) {
        logError(config, "Failed to get function pointer for: " + functionName);
        throw std::runtime_error("Failed to get function pointer");
    }

    DEBUG_LOG("Executing JIT-compiled function: " + functionName);
    auto startTime = std::chrono::high_resolution_clock::now();

    int result = fn();

    if (config.diagnostics.logTimings) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        DEBUG_LOG("Execution time: " + std::to_string(duration.count()) + " microseconds");
    }
    if (config.diagnostics.verbose) {
        DEBUG_LOG("Function returned: " + std::to_string(result));
    }
}

llvm::Expected<llvm::orc::ExecutorAddr> LLVMJITBackend::lookupFunction(const std::string& name) {
    return jit->lookup(name);
}

bool LLVMJITBackend::isCompiled(const std::string& functionName) const {
    return static_cast<bool>(jit->lookup(functionName));
}

void LLVMJITBackend::compileFunction(const std::string& functionName) {
    auto symbol = jit->lookup(functionName);
    if (!symbol) {
        logError(Config(), "Function not found for compilation: " + functionName);
        throw std::runtime_error("Function not found for compilation");
    }
    // Implement lazy compilation if needed
    DEBUG_LOG("Compiled function: " + functionName);
}

void LLVMJITBackend::invalidateFunction(const std::string& functionName) {
    DEBUG_LOG("Invalidating function: " + functionName);
    // Implement symbol removal if supported by LLJIT
    codeCacheSize = std::max(static_cast<size_t>(0), codeCacheSize - estimateFunctionSize(functionName));
}

void LLVMJITBackend::clearCodeCache() {
    DEBUG_LOG("Clearing JIT code cache");
    // Implement cache clearing if supported
    codeCacheSize = 0;
}

size_t LLVMJITBackend::getCodeCacheSize() const {
    return codeCacheSize;
}

void LLVMJITBackend::setCodeCacheLimit(size_t limit) {
    DEBUG_LOG("Setting code cache limit to: " + std::to_string(limit) + " bytes");
    // Implement cache limit setting if supported
}

void LLVMJITBackend::cleanup() {
    finalizeProfiler(Config());
    clearCodeCache();
    jit.reset();
    irGen.reset();
    scope.reset();
    fileCache.clear();
    peakMemoryUsage = 0;
    profilerInitialized = false;
    DEBUG_LOG("LLVMJITBackend cleaned up");
}

void LLVMJITBackend::updateFileCache(const Config& config) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto sourceFile = config.mainSourceFile.empty() ? config.filePath : config.mainSourceFile;
    if (!sourceFile.empty()) {
        try {
            fileCache[sourceFile] = std::filesystem::last_write_time(sourceFile);
        } catch (const std::filesystem::filesystem_error& e) {
            logError(config, "Failed to update file cache: " + std::string(e.what()));
        }
    }
    for (const auto& path : config.sourcePaths) {
        try {
            fileCache[path] = std::filesystem::last_write_time(path);
        } catch (const std::filesystem::filesystem_error& e) {
            logError(config, "Failed to update file cache for " + path + ": " + std::string(e.what()));
        }
    }
}

bool LLVMJITBackend::isFileUpToDate(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = fileCache.find(filePath);
    if (it == fileCache.end()) {
        return false;
    }
    try {
        return std::filesystem::last_write_time(filePath) <= it->second;
    } catch (const std::filesystem::filesystem_error& e) {
        logError(Config(), "Failed to check file timestamp: " + std::string(e.what()));
        return false;
    }
}

void LLVMJITBackend::logError(const Config& config, const std::string& message) {
    console.error(message);
    if (config.errorHandling.logToFile && !config.errorHandling.errorLogPath.empty()) {
        std::ofstream logFile(config.errorHandling.errorLogPath, std::ios::app);
        if (logFile.is_open()) {
            logFile << "[" << std::time(nullptr) << "] ERROR: " + message + "\n";
            logFile.close();
        }
    }
}

void LLVMJITBackend::emitAOTOutput(std::unique_ptr<llvm::Module> module, const std::string& outputPath, const Config& config) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(outputPath, ec, llvm::sys::fs::OF_None);
    if (ec) {
        logError(config, "Failed to open AOT output file: " + ec.message());
        throw std::runtime_error("Failed to open AOT output file");
    }

    auto jtmbExpected = createTargetMachineBuilder(config);
    if (!jtmbExpected) {
        logError(config, "Failed to create JIT target machine builder: " + llvm::toString(jtmbExpected.takeError()));
        throw std::runtime_error("Failed to create JIT target machine builder");
    }

    auto jtmb = std::move(*jtmbExpected);
    
    // Store triple and create target machine before moving jtmb
    std::string targetTriple = jtmb.getTargetTriple().str();
    
    // Create target machine to get data layout
    auto targetMachineExpected = jtmb.createTargetMachine();
    if (!targetMachineExpected) {
        logError(config, "Failed to create target machine: " + llvm::toString(targetMachineExpected.takeError()));
        throw std::runtime_error("Failed to create target machine");
    }
    
    auto targetMachine = std::move(*targetMachineExpected);
    auto dataLayout = targetMachine->createDataLayout();
    
    // Set module properties
    module->setTargetTriple(llvm::Triple(targetTriple));
    module->setDataLayout(dataLayout);
    
    // Create JIT for AOT compilation
    auto jitOrError = llvm::orc::LLJITBuilder()
        .setJITTargetMachineBuilder(std::move(jtmb))
        .create();

    if (!jitOrError) {
        logError(config, "Failed to create LLJIT: " + llvm::toString(jitOrError.takeError()));
        throw std::runtime_error("Failed to create LLJIT");
    }

    auto& jitInstance = *jitOrError;
    
    // Create ThreadSafeContext - you can either create a new one or use the IRGenerator's context
    auto threadSafeContext = std::make_unique<llvm::orc::ThreadSafeContext>(std::make_unique<llvm::LLVMContext>());
    
    // Add module to JIT - use proper method calls
    auto tsm = llvm::orc::ThreadSafeModule(std::move(module), *threadSafeContext);
    if (auto err = jitInstance->addIRModule(std::move(tsm))) {
        logError(config, "Failed to add module to JIT: " + llvm::toString(std::move(err)));
        throw std::runtime_error("Failed to add module to JIT");
    }
    
    // Additional AOT compilation logic can be added here
    // For now, this creates a JIT-compiled version for hybrid mode
    dest.close();
}

size_t LLVMJITBackend::estimateFunctionSize(const std::string& functionName) {
    // Placeholder: Estimate function size based on IR or compiled code size
    return 1024; // Dummy value for demonstration
}

void LLVMJITBackend::executePluginCallbacks(const Config& config, const std::string& stage) {
    for (const auto& plugin : config.plugins) {
        if (plugin.callback) {
            try {
                plugin.callback(config, const_cast<void*>(static_cast<const void*>(&stage)));
            } catch (const std::exception& e) {
                logError(config, "Plugin " + plugin.name + " failed at stage " + stage + ": " + e.what());
            }
        }
    }
}

void LLVMJITBackend::trackMemoryUsage(const Config& config) {
    size_t currentUsage = 0;
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS memInfo;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &memInfo, sizeof(memInfo))) {
        currentUsage = memInfo.PeakWorkingSetSize;
    }
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        currentUsage = usage.ru_maxrss * 1024;
    }
#endif
    peakMemoryUsage = std::max(peakMemoryUsage, currentUsage);
    if (config.diagnostics.measureMemoryUsage) {
        DEBUG_LOG("Current memory usage: " + std::to_string(currentUsage / 1024 / 1024) + " MB");
    }
}

bool LLVMJITBackend::checkTimeLimit(const Config& config) {
    if (config.maxCompilationTime == 0) {
        return true;
    }
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - compilationStartTime).count();
    if (elapsed >= static_cast<long>(config.maxCompilationTime)) {
        std::string message = Console::formatString(
            "Compilation stopped because it took %ld seconds, exceeding your limit of %u seconds.",
            elapsed, config.maxCompilationTime
        );
        std::string suggestion = "To fix this:\n"
                                "1. Increase the time limit with `--max-compilation-time [seconds]` (e.g., `--max-compilation-time 30`).\n"
                                "2. Simplify your code to reduce complexity (e.g., fewer nested structs or functions).\n"
                                "3. Use `--parallel-jobs [n]` to speed up compilation with multiple threads.\n"
                                "See compiler options at: https://omniscript.dev/docs";
        console.reportError(Console::ErrorType::RUNTIME_ERROR, message, suggestion);
        throw std::runtime_error("Compilation time limit exceeded");
    }
    return true;
}

} // namespace Omniscript