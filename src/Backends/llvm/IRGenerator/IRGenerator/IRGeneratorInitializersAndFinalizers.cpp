#include <omniscript/Backends/LLVM/IRGenerator.h>

void IRGenerator::finalize() {
    // Find the top-level function
    llvm::Function* topFunc = Module->getFunction("__top_level__");
    if (!topFunc) {
        llvm::errs() << "Warning: No top-level function found.\n";
        return;
    }

    // Handle empty function case
    if (topFunc->empty()) {
        llvm::BasicBlock* entry = llvm::BasicBlock::Create(*Context, "entry", topFunc);
        Builder->SetInsertPoint(entry);
        Builder->CreateRetVoid();
        return;
    }

    // Get the last basic block
    llvm::BasicBlock* lastBlock = &topFunc->back();

    // If the block has no terminator, add a `ret void`
    if (!lastBlock->getTerminator()) {
        Builder->SetInsertPoint(lastBlock);
        
        // Check if we're in the middle of a block (unlikely but possible)
        if (Builder->GetInsertBlock() != lastBlock) {
            Builder->SetInsertPoint(lastBlock);
        }
        
        Builder->CreateRetVoid();
    }
}

void IRGenerator::initialize() {
    DEBUG_LOG("Initializing The IR Generator");
    // Validate configuration first
    std::string validationError;
    if (!configs.validate(validationError)) {
        llvm::errs() << "Configuration validation failed: " << validationError << "\n";
        return;
    }
    
    // Auto-configure for target if not already done
    // This should ideally be called when configs is first set up, but safe to call again
    const_cast<Config&>(configs).autoConfigureForTarget();
    
    // Create context, module, builder if not yet created
    if (!Context)
        Context = std::make_unique<llvm::LLVMContext>();
    
    if (!Module) {
        // Use config file path as module name, or default
        std::string moduleName = "OmniScript";
        if (!configs.filePath.empty()) {
            // Extract filename without extension
            size_t lastSlash = configs.filePath.find_last_of("/\\");
            size_t lastDot = configs.filePath.find_last_of('.');
            if (lastSlash != std::string::npos && lastDot != std::string::npos) {
                moduleName = configs.filePath.substr(lastSlash + 1, lastDot - lastSlash - 1);
            }
        }
        Module = std::make_unique<llvm::Module>(moduleName, *Context);
    }
    
    if (!Builder)
        Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
    
    // Initialize target based on configuration
    initializeTargetFromConfig();
    
    // Set up module metadata based on configuration
    setupModuleMetadata();
    
    // Initialize optimization passes if needed
    if (configs.optimization.level > 0) {
        setupOptimizationPipeline();
    }
    
    // Set up debugging information if enabled
    if (configs.aot.generateDebugInfo || configs.diagnostics.debugMode) {
        setupDebugInfo();
    }
    
    // Create entry function based on configuration
    createEntryFunction();
    
    // Set up external resolvers based on target OS
    setupExternalResolvers();
    
    currentModule = Module.get();
    
    // Print configuration summary if verbose mode is enabled
    if (configs.diagnostics.verbose) {
        configs.printSummary();
    }
}

void IRGenerator::initializeTargetFromConfig() {
    DEBUG_LOG("Initializing The target from the configurations");
    // Initialize target based on config
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();

    std::string error;
    std::string triple = configs.getEffectiveTargetTriple();
    
    if (configs.diagnostics.verbose) {
        llvm::outs() << "Using target triple: " << triple << "\n";
    }
    
    Module->setTargetTriple(triple);
    
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple, error);
    if (!target) {
        llvm::errs() << "Target lookup failed: " << error << "\n";
        return;
    }
    
    // Configure target options based on security and optimization settings
    llvm::TargetOptions options;
    
    // Security configurations
    if (configs.security.enableStackProtection) {
        // options.StackProtectorLevel = 2; // Strong stack protection
    }
    
    if (configs.security.enableControlFlowIntegrity) {
        // Enable CFI-related options
        // options.CFIntegrity = true;
    }
    
    // Optimization-related target options
    if (configs.optimization.fastMath) {
        options.UnsafeFPMath = true;
        options.NoInfsFPMath = true;
        options.NoNaNsFPMath = true;
        llvm::FastMathFlags fmf;
        fmf.setFast(); // enables Unsafe, NoNaNs, NoInfs, NoSignedZeros, etc.
        Builder->setFastMathFlags(fmf);
    }
    
    // Use TargetInfo to resolve CPU and features properly
    std::string cpu = resolveCPUName(triple);
    std::string features = buildFeatureString(triple);
    
    if (configs.diagnostics.verbose) {
        llvm::outs() << "Using CPU: " << cpu << "\n";
        if (!features.empty()) {
            llvm::outs() << "Using features: " << features << "\n";
        }
    }
    
    // Determine relocation model
    std::optional<llvm::Reloc::Model> relocModel = std::nullopt;
    if (configs.security.enablePositionIndependentCode) {
        relocModel = llvm::Reloc::PIC_;
    }

    llvm::CodeGenOptLevel optLevel = llvm::CodeGenOptLevel::None;
    switch (configs.optimization.level) {
        case 1: optLevel = llvm::CodeGenOptLevel::Less; break;
        case 2: optLevel = llvm::CodeGenOptLevel::Default; break;
        case 3: optLevel = llvm::CodeGenOptLevel::Aggressive; break;
        default: optLevel = llvm::CodeGenOptLevel::None; break;
    }
    
    // Validate target triple compatibility before creating target machine
    if (!validateTargetTripleCompatibility(triple, cpu)) {
        llvm::errs() << "Target triple and CPU are incompatible\n";
        return;
    }
    
    auto targetMachine = std::unique_ptr<llvm::TargetMachine>(
        target->createTargetMachine(triple, cpu, features, options, relocModel, 
                                   std::nullopt, optLevel)
    );
    
    if (!targetMachine) {
        llvm::errs() << "Failed to create target machine for triple: " << triple 
                     << ", CPU: " << cpu << ", features: " << features << "\n";
        return;
    }
    
    // Verify the target machine supports the requested architecture
    if (configs.diagnostics.verbose) {
        llvm::outs() << "Target machine created successfully\n";
        llvm::outs() << "Data layout: " << targetMachine->createDataLayout().getStringRepresentation() << "\n";
    }
    
    Module->setDataLayout(targetMachine->createDataLayout());
    
    // Store target machine for later use in compilation
    this->targetMachine = std::move(targetMachine);
}
