#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Backends/llvm/IRGenerator.h>
#include <omniscript/engine/Backends/llvm/LLVMJITBackend.h>

// Implementation
LLVMJITBackend::LLVMJITBackend() {
    scope = std::make_shared<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>>();
}

void LLVMJITBackend::initialize() {
    // Basic initialization - full setup happens in setupJITEngine
}

void LLVMJITBackend::setupJITEngine(const Config& config) {
    DEBUG_LOG("Setting up the JIT engine");
    // Validate JIT configuration
    if (config.isCrossCompilation()) {
        throw std::runtime_error("JIT compilation is not supported for cross-compilation targets");
    }
    
    // Create target machine builder based on config
    auto jtmbExpected = createTargetMachineBuilder(config);
    if (!jtmbExpected) {
        throw std::runtime_error("Failed to create JIT target machine builder: " + 
                                llvm::toString(jtmbExpected.takeError()));
    }
    auto jtmb = std::move(*jtmbExpected);
    
    // Configure JIT engine type
    switch (config.jit.engine) {
        case JITEngine::LLVM_ORC: {
            auto jitOrError = llvm::orc::LLJITBuilder()
                .setJITTargetMachineBuilder(std::move(jtmb))
                .create();
            
            if (!jitOrError) {
                throw std::runtime_error("Failed to create LLJIT: " + 
                    llvm::toString(jitOrError.takeError()));
            }
            
            jit = std::move(jitOrError.get());
            break;
        }
        
        case JITEngine::LLVM_MCJIT:
            // Legacy MCJIT setup (if needed)
            throw std::runtime_error("MCJIT engine is deprecated, use LLVM_ORC instead");
            
        case JITEngine::Custom:
            // Custom JIT implementation
            throw std::runtime_error("Custom JIT engine not implemented yet");
            
        default:
            throw std::runtime_error("Unknown JIT engine type");
    }
    
    // Configure JIT-specific options
    configureJITOptions(config);
    
    DEBUG_LOG("JIT engine initialized with target: " + config.getTargetSummary());
}

llvm::Expected<llvm::orc::JITTargetMachineBuilder> LLVMJITBackend::createTargetMachineBuilder(const Config& config) {
    // detectHost() returns Expected<JITTargetMachineBuilder>, not JITTargetMachineBuilder
    auto jtmbExpected = llvm::orc::JITTargetMachineBuilder::detectHost();
    if (!jtmbExpected) {
        // Return the error instead of throwing - this is an Expected<T> function
        return jtmbExpected.takeError();
    }
    
    // Extract the actual builder from Expected<T>
    auto builder = std::move(*jtmbExpected);
    
    // Set optimization level
    switch (config.optimization.level) {
        case 0: builder.setCodeGenOptLevel(llvm::CodeGenOptLevel::None); break;
        case 1: builder.setCodeGenOptLevel(llvm::CodeGenOptLevel::Less); break;
        case 2: builder.setCodeGenOptLevel(llvm::CodeGenOptLevel::Default); break;
        case 3: builder.setCodeGenOptLevel(llvm::CodeGenOptLevel::Aggressive); break;
        default: builder.setCodeGenOptLevel(llvm::CodeGenOptLevel::Default); break;
    }
    
    // Configure target options
    llvm::TargetOptions opt;
    opt.UnsafeFPMath = config.optimization.fastMath;
    opt.NoInfsFPMath = config.optimization.fastMath;
    opt.NoNaNsFPMath = config.optimization.fastMath;
    opt.GuaranteedTailCallOpt = config.optimization.enableTailCallOptimization;
    
    if (config.runtime.enableParallelGC || config.runtime.gcThreads > 1) {
        opt.ThreadModel = llvm::ThreadModel::POSIX;
    } else {
        opt.ThreadModel = llvm::ThreadModel::Single;
    }
    
    builder.setOptions(opt);
    
    // Fix: Always resolve CPU name using IRGenerator's function
    // Get the target triple from the builder
    std::string targetTriple = builder.getTargetTriple().str();
    
    // Use IRGenerator's resolveCPUName function
    std::string resolvedCPU = irGen->resolveCPUName(targetTriple);
    builder.setCPU(resolvedCPU);
    
    // Optional: Also set features if you have a buildFeatureString function
    std::string features = irGen->buildFeatureString(targetTriple);
    if (!features.empty()) {
        // Parse comma-separated features into vector
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
    // Configure lazy compilation
    if (config.jit.lazyCompilation) {
        // Enable lazy compilation if supported by the JIT engine
        DEBUG_LOG("Lazy compilation enabled");
    }
    
    // Configure code cache size
    if (config.jit.maxCodeCacheSize > 0) {
        setCodeCacheLimit(config.jit.maxCodeCacheSize);
    }
    
    // Configure compilation threshold
    if (config.jit.compilationThreshold > 0) {
        DEBUG_LOG("Compilation threshold set to: " + std::to_string(config.jit.compilationThreshold));
    }
    
    // Configure tiered compilation
    if (config.jit.enableTieredCompilation) {
        DEBUG_LOG("Tiered compilation enabled");
    }
    
    // Configure debugging support
    if (config.jit.enableDebugging || config.diagnostics.debugMode) {
        DEBUG_LOG("JIT debugging support enabled");
    }
}

void LLVMJITBackend::setupExternalResolvers(const Config& config) {
    auto targetOS = config.resolveTargetOS();
    
    DEBUG_LOG("Setting up JIT external resolvers for: " + config.getOSName());
    
    // Add host process symbol resolver
    auto hostResolver = createHostProcessResolver(config);
    if (hostResolver) {
        jit->getMainJITDylib().addGenerator(std::move(hostResolver));
    }
    
    // Set up IR generator with external resolvers (similar to AOT backend)
    irGen->addExternalResolver("C", std::make_unique<CStdLibResolver>());
    
    // Platform-specific resolvers
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
    
    // Register runtime symbols
    registerRuntimeSymbols(config);
    
    DEBUG_LOG("JIT external resolvers setup completed");
}

std::unique_ptr<llvm::orc::DefinitionGenerator> LLVMJITBackend::createHostProcessResolver(const Config& config) {
    // Create a symbol resolver that can find symbols in the host process
    auto resolver = llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
        jit->getDataLayout().getGlobalPrefix());
    
    if (!resolver) {
        DEBUG_LOG("Warning: Failed to create host process resolver: " + 
            llvm::toString(resolver.takeError()));
        return nullptr;
    }
    
    return std::move(resolver.get());
}

void LLVMJITBackend::registerRuntimeSymbols(const Config& config) {
    // Register runtime symbols that might be needed by the JIT
    auto& mainDylib = jit->getMainJITDylib();
    
    // Register garbage collection symbols if needed
    if (config.runtime.gcStrategy != GCStrategy::None) {
        // Register GC-related symbols
        DEBUG_LOG("Registering GC runtime symbols");
    }
    
    // Register safety check symbols if needed
    if (config.runtime.safetyLevel > SafetyLevel::Unsafe) {
        // Register safety check runtime symbols
        DEBUG_LOG("Registering safety check symbols");
    }
}

void LLVMJITBackend::execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config& config) {
    DEBUG_LOG();
    DEBUG_LOG("Executing with LLVM JIT Backend");
    DEBUG_LOG("===============================");
    
    // Validate configuration
    std::string validationError;
    if (!config.validate(validationError)) {
        throw std::runtime_error("Configuration validation failed: " + validationError);
    }
    
    if (config.diagnostics.verbose) {
        config.printSummary();
    }
    
    scope->setName(config.filePath);
    irGen = std::make_shared<IRGenerator>(config);

    // Setup JIT engine with config
    setupJITEngine(config);
    
    setupExternalResolvers(config);
    
    DEBUG_LOG("Evaluating statements for JIT compilation");
    DEBUG_LOG("=========================================");
    
    // Process statements
    for (const auto& statement : statements) {
        DEBUG_LOG("Evaluating " + statement->toString());
        Omniscript::setPosition(statement->getPosition());
        auto expr = statement->express(scope);
        if (!expr) continue;
        
        DEBUG_LOG("Generating LLVM IR for: " + expr->toString());
        irGen->codegen(expr, scope);
    }
    
    irGen->compileAllFunctionBodies(scope);
    irGen->finalizeGlobalInitializers();
    irGen->finalize();
    
    if (config.diagnostics.logFinalCode) {
        console.log("========= JIT LLVM IR =========");
        irGen->printIR();
    }
    
    irGen->printErrors();
    
    // Optimize module based on config
    if (config.optimization.level > 0) {
        irGen->optimizeModule(config.optimization.level);
        
        if (config.diagnostics.logFinalCode) {
            console.log("========= Optimized JIT LLVM IR =========");
            irGen->printIR();
        }
    }
    
    // Determine entry point BEFORE moving module to JIT
    // Priority: 1. config.entry, 2. main function, 3. __top_level__
    std::string entryPoint;
    
    if (!config.entry.empty()) {
        entryPoint = config.entry;
        DEBUG_LOG("Using configured entry point: " + entryPoint);
    } else {
        // Check if there's a main function with correct signature
        if (hasMainFunction()) {
            entryPoint = "__main";
            DEBUG_LOG("Found main function with correct signature");
        } else {
            entryPoint = "__top_level__";
            DEBUG_LOG("Using default entry point: __top_level__");
        }
    }
    
    // Add module to JIT
    auto module = irGen->getModule();
    auto tsm = llvm::orc::ThreadSafeModule(std::move(module), irGen->getContext());
    
    auto err = jit->addIRModule(std::move(tsm));
    if (err) {
        throw std::runtime_error("Failed to add module to JIT: " + llvm::toString(std::move(err)));
    }
    
    // Execute the entry point
    executeFunction(entryPoint, config);
}

bool LLVMJITBackend::hasMainFunction() {
    auto module = irGen->getCurrentModule();
    if (!module) return false;

    auto mainFunc = module->getFunction("__main");
    if (!mainFunc) return false;

    auto funcType = mainFunc->getFunctionType();

    // Must return i32
    if (!funcType->getReturnType()->isIntegerTy(32)) return false;

    // Accept either: int main() or int main(int, char**)
    unsigned numParams = funcType->getNumParams();
    if (numParams == 0) {
        return true;
    } else if (numParams == 2) {
        return funcType->getParamType(0)->isIntegerTy(32) &&
               funcType->getParamType(1)->isPointerTy();
    }

    return false;
}


void LLVMJITBackend::executeFunction(const std::string& functionName, const Config& config) {
    auto symbol = jit->lookup(functionName);
    if (!symbol) {
        throw std::runtime_error("Function '" + functionName + "' not found: " + 
            llvm::toString(symbol.takeError()));
    }
    
    auto fnAddr = symbol.get();
    
    // Cast to function pointer and execute
    auto fn = fnAddr.toPtr<int()>();
    if (!fn) {
        throw std::runtime_error("Failed to get function pointer for: " + functionName);
    }
    
    DEBUG_LOG("Executing JIT-compiled function: " + functionName);
    
    // Measure execution time if diagnostics are enabled
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
    auto symbol = jit->lookup(functionName);
    return static_cast<bool>(symbol);
}

void LLVMJITBackend::compileFunction(const std::string& functionName) {
    // Trigger compilation of a specific function
    auto symbol = jit->lookup(functionName);
    if (!symbol) {
        throw std::runtime_error("Function not found for compilation: " + functionName);
    }
}

void LLVMJITBackend::invalidateFunction(const std::string& functionName) {
    // Remove function from JIT cache to force recompilation
    DEBUG_LOG("Invalidating function: " + functionName);
}

void LLVMJITBackend::clearCodeCache() {
    DEBUG_LOG("Clearing JIT code cache");
    // Implementation depends on JIT engine
}

size_t LLVMJITBackend::getCodeCacheSize() const {
    // Return current code cache size
    return 0; // Implementation depends on JIT engine
}

void LLVMJITBackend::setCodeCacheLimit(size_t limit) {
    DEBUG_LOG("Setting code cache limit to: " + std::to_string(limit) + " bytes");
    // Implementation depends on JIT engine
}

void LLVMJITBackend::cleanup() {
    if (jit) {
        clearCodeCache();
        jit.reset();
    }
    // scope.reset();
    // irGen.reset();
}