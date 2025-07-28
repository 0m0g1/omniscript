#include <omniscript/Backends/llvm/LLVMJITBackend.h>
#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/Program.h>
#include <llvm/IR/Module.h>
#include <thread>
#include <fstream>
#include <filesystem>

namespace Omniscript {

LLVMJITBackend::LLVMJITBackend() {
    scope = std::make_shared<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>>();
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
    opt.UnsafeFPMath = config.optimization.fastMath;
    opt.NoInfsFPMath = config.optimization.fastMath;
    opt.NoNaNsFPMath = config.optimization.fastMath;
    opt.NoSignedZerosFPMath = config.optimization.fastMath;
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

    size_t errorCount = 0;
    for (const auto& statement : statements) {
        if (!checkTimeLimit(config) || peakMemoryUsage > config.maxMemoryUsage) {
            logError(config, "Compilation aborted: Time or memory limit exceeded");
            throw std::runtime_error("Compilation limits exceeded");
        }

        DEBUG_LOG("Evaluating " + statement->toString());
        Omniscript::setSpan(statement->getSpan());
        auto expr = statement->express(scope);
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
        irGen->codegen(expr, scope);
        trackMemoryUsage(config);
    }

    irGen->compileAllFunctionBodies(scope);
    irGen->finalizeGlobalInitializers();
    irGen->finalize();

    if (config.diagnostics.logFinalCode) {
        console.log("========= JIT LLVM IR =========");
        irGen->printIR();
    }

    irGen->printErrors();

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

    auto module = irGen->getModule();
    auto tsm = llvm::orc::ThreadSafeModule(std::move(module), irGen->getContext());
    auto err = jit->addIRModule(std::move(tsm));
    if (err) {
        logError(config, "Failed to add module to JIT: " + llvm::toString(std::move(err)));
        throw std::runtime_error("Failed to add module to JIT");
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
    module->setTargetTriple(targetTriple);
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
    return elapsed < static_cast<long>(config.maxCompilationTime);
}

} // namespace Omniscript