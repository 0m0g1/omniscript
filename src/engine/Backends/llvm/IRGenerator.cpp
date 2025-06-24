#include <omniscript/Core/CPUFeatures.h>
#include <omniscript/engine/Backends/llvm/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/CLLVMResolver.h>

#include <llvm/ADT/StringMap.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/Alignment.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Linker/Linker.h>


IRGenerator::IRGenerator(const Config& configs) {
    this->configs = configs;
    Context = std::make_unique<llvm::LLVMContext>();
    Module = std::make_unique<llvm::Module>(configs.filePath, *Context);
    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
    initialize();
}

bool IRGenerator::supportsAVX512() {
    // llvm::StringMap<bool> Features;
    // llvm::sys::getHostCPUFeatures(Features);
    // return Features.lookup("avx512f"); // Check if AVX-512 is supported
    return false;
}

bool IRGenerator::supportsAVX2() {
    // llvm::StringMap<bool> Features;
    // llvm::sys::getHostCPUFeatures(Features);
    // return Features.lookup("avx2"); // Check if AVX2 is supported
    return false;
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

// Use TargetInfo utilities to resolve CPU name properly
std::string IRGenerator::resolveCPUName(const std::string& triple) {
    if (configs.cpuFeatures == "native") {
        // Use LLVM's host CPU detection for native
        std::string hostCPU = llvm::sys::getHostCPUName().str();
        if (!hostCPU.empty() && hostCPU != "generic") {
            return hostCPU;
        }
        
        // Fallback to TargetInfo's default CPU for detected architecture
        TargetInfo::TargetTriple parsedTriple = TargetInfo::TargetTriple::parse(triple);
        TargetArch arch = getTargetArchFromTriple(parsedTriple);
        return TargetInfo::getDefaultCPUForArch(arch);
    } else if (!configs.cpuFeatures.empty()) {
        return configs.cpuFeatures;
    } else {
        // Use TargetInfo to get default CPU for the architecture
        TargetInfo::TargetTriple parsedTriple = TargetInfo::TargetTriple::parse(triple);
        TargetArch arch = getTargetArchFromTriple(parsedTriple);
        return TargetInfo::getDefaultCPUForArch(arch);
    }
}

// Build feature string using TargetInfo and user configuration
std::string IRGenerator::buildFeatureString(const std::string& triple) {
    DEBUG_LOG("Building the features string");
    std::string features;
    
    // Add explicitly enabled features
    for (const auto& feature : configs.enabledFeatures) {
        if (!features.empty()) features += ",";
        features += "+" + feature;
    }
    
    // Add explicitly disabled features
    for (const auto& feature : configs.disabledFeatures) {
        if (!features.empty()) features += ",";
        features += "-" + feature;
    }
    
    // Add host features only if CPU is native and no features specified manually
    if (configs.cpuFeatures == "native" && 
        configs.enabledFeatures.empty() && 
        configs.disabledFeatures.empty()) {
        
        // Use the integrated feature detection with target validation
        auto hostFeatures = CPUFeatures::getHostCPUFeatures();
        
        for (const auto& feature : hostFeatures) {
            if (feature.second) { // Feature is enabled and valid for target
                const std::string& featureName = feature.first;
                if (!features.empty()) features += ",";
                features += "+" + featureName;
            }
        }
            
        
        DEBUG_LOG("Done getting the features");
    }
    
    return features;
}

// Convert LLVM triple to TargetInfo TargetArch enum
TargetArch IRGenerator::getTargetArchFromTriple(const TargetInfo::TargetTriple& triple) {
    std::string arch = triple.arch;
    std::transform(arch.begin(), arch.end(), arch.begin(), ::tolower);
    
    if (arch == "x86_64" || arch == "x86-64" || arch == "amd64") {
        return TargetArch::X86_64;
    } else if (arch == "aarch64" || arch == "arm64") {
        return TargetArch::ARM64;
    } else if (arch == "i386" || arch == "i486" || arch == "i586" || arch == "i686") {
        return TargetArch::X86_32;
    } else if (arch == "arm" || arch == "armv6" || arch == "armv7") {
        return TargetArch::ARM32;
    } else if (arch == "riscv64") {
        return TargetArch::RISCV64;
    } else if (arch == "wasm32") {
        return TargetArch::WASM32;
    } else if (arch == "wasm64") {
        return TargetArch::WASM64;
    } else {
        return TargetArch::Auto; // Will auto-detect
    }
}

// Validate target triple compatibility using TargetInfo
bool IRGenerator::validateTargetTripleCompatibility(const std::string& triple, const std::string& cpu) {
    // First, use TargetInfo to validate the triple format
    if (!TargetInfo::isValidTriple(triple)) {
        llvm::errs() << "Invalid target triple format: " << triple << "\n";
        return false;
    }
    
    // Parse and normalize the triple
    TargetInfo::TargetTriple normalized = TargetInfo::normalizeTriple(triple);
    TargetArch arch = getTargetArchFromTriple(normalized);
    
    // Check architecture-specific constraints
    if (arch == TargetArch::X86_64) {
        // For x86_64 targets, ensure we're not using 32-bit only CPUs
        static const std::unordered_set<std::string> incompatibleCPUs = {
            "i386", "i486", "i586"
        };
        
        if (incompatibleCPUs.find(cpu) != incompatibleCPUs.end()) {
            llvm::errs() << "Warning: CPU '" << cpu 
                         << "' may not support 64-bit mode for triple '" << triple << "'\n";
            return false;
        }
    }
    
    // Additional validation using TargetInfo
    TargetInfo::ArchitectureInfo archInfo = TargetInfo::getArchitectureInfo(arch);
    
    // Check if the triple requests 64-bit but architecture doesn't support it
    bool tripleIs64Bit = (normalized.arch.find("64") != std::string::npos);
    if (tripleIs64Bit && !archInfo.is64Bit) {
        llvm::errs() << "Target triple requests 64-bit but architecture '" 
                     << archInfo.name << "' doesn't support it\n";
        return false;
    }
    
    // Check if the triple requests 32-bit but we're on 64-bit only arch
    bool tripleIs32Bit = (normalized.arch.find("32") != std::string::npos || 
                         normalized.arch == "i386" || normalized.arch == "arm");
    if (tripleIs32Bit && archInfo.is64Bit && archInfo.name == "arm64") {
        // ARM64 can run ARM32, so this is OK
    } else if (tripleIs32Bit && archInfo.is64Bit && archInfo.name != "x86_64") {
        llvm::errs() << "Target triple requests 32-bit but architecture '" 
                     << archInfo.name << "' is 64-bit only\n";
        return false;
    }
    
    if (configs.diagnostics.verbose) {
        llvm::outs() << "Target validation passed:\n";
        llvm::outs() << "  Normalized triple: " << normalized.toString() << "\n";
        llvm::outs() << "  Architecture: " << archInfo.name << " (" << archInfo.pointerSize << "-byte pointers)\n";
        llvm::outs() << "  CPU: " << cpu << "\n";
    }
    
    return true;
}

void IRGenerator::setupModuleMetadata() {
    DEBUG_LOG("Setting up the module's metadata");
    // Add module metadata based on configuration
    llvm::LLVMContext& ctx = *Context;
    
    // Add source file information
    if (!configs.filePath.empty()) {
        llvm::Metadata* sourceFile = llvm::MDString::get(ctx, configs.filePath);
        Module->addModuleFlag(llvm::Module::Warning, "source.file", sourceFile);
    }
    
    // Add language standard information
    llvm::Metadata* langStd = llvm::MDString::get(ctx, configs.languageStandard);
    Module->addModuleFlag(llvm::Module::Warning, "language.standard", langStd);
    
    // Add compilation mode
    std::string modeStr = configs.isJITMode() ? "jit" : "aot";
    llvm::Metadata* mode = llvm::MDString::get(ctx, modeStr);
    Module->addModuleFlag(llvm::Module::Warning, "compile.mode", mode);
    
    // Add safety level
    llvm::Metadata* safety = llvm::ConstantAsMetadata::get(
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), static_cast<int>(configs.runtime.safetyLevel))
    );
    Module->addModuleFlag(llvm::Module::Warning, "safety.level", safety);
    
    // Add PIC flag if enabled
    if (configs.security.enablePositionIndependentCode) {
        llvm::Metadata* pic = llvm::ConstantAsMetadata::get(
            llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx), 1)
        );
        Module->addModuleFlag(llvm::Module::Error, "PIC Level", pic);
    }
}

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

void IRGenerator::setupDebugInfo() {
    DEBUG_LOG("Setting up debugging info, (does nothing for now)");
    // if (!configs.aot.generateDebugInfo && !configs.diagnostics.debugMode) {
    //     return;
    // }
    
    // // Create debug info builder
    // auto debugBuilder = std::make_unique<llvm::DIBuilder>(*Module);
    
    // // Create compile unit
    // std::string filename = configs.filePath.empty() ? "unknown" : configs.filePath;
    // size_t lastSlash = filename.find_last_of("/\\");
    // std::string directory = (lastSlash != std::string::npos) ? filename.substr(0, lastSlash) : ".";
    // std::string file = (lastSlash != std::string::npos) ? filename.substr(lastSlash + 1) : filename;
    
    // auto debugCompileUnit = debugBuilder->createCompileUnit(
    //     llvm::dwarf::DW_LANG_C,  // You might want to define your own language constant
    //     debugBuilder->createFile(file, directory),
    //     "OmniScript Compiler",
    //     configs.optimization.level > 0,  // isOptimized
    //     "",  // flags
    //     0    // runtime version
    // );
}

void IRGenerator::createEntryFunction() {
    DEBUG_LOG("creating the entry function '__top_level'.");
    // Always create __top_level__ as the entry point
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(*Context), 
        false  // no parameters
    );
    
    llvm::Function* entryFunc = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        "__top_level__",
        Module.get()
    );
    
    // Set function attributes based on configuration
    if (configs.mode != CompileMode::JIT) {
        if (configs.security.enableStackProtection) {
            // entryFunc->addFnAttr(llvm::Attribute::StackProtectReq);
        }
    }
    
     if (configs.optimization.level == 0) {
        entryFunc->addFnAttr(llvm::Attribute::OptimizeForSize);     
    }
    
    // Create entry basic block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*Context, "entry", entryFunc);
    Builder->SetInsertPoint(entry);
}

void IRGenerator::setupExternalResolvers() {
    DEBUG_LOG("Setting up the external resolvers");
    // Set up external resolvers based on target OS
    auto targetOS = configs.resolveTargetOS();
    
    // Always add C standard library resolver
    addExternalResolver("C", std::make_unique<CStdLibResolver>());
    
    // Add OS-specific resolvers
    // switch (targetOS) {
    //     case TargetOS::Windows:
    //         addExternalResolver("Win32", std::make_unique<Win32Resolver>());
    //         break;
    //     case TargetOS::Linux:
    //     case TargetOS::Ubuntu:
    //     case TargetOS::Debian:
    //         addExternalResolver("POSIX", std::make_unique<PosixResolver>());
    //         addExternalResolver("Linux", std::make_unique<LinuxResolver>());
    //         break;
    //     case TargetOS::macOS:
    //         addExternalResolver("POSIX", std::make_unique<PosixResolver>());
    //         addExternalResolver("Darwin", std::make_unique<DarwinResolver>());
    //         break;
    //     case TargetOS::WebAssembly:
    //         addExternalResolver("WASM", std::make_unique<WasmResolver>());
    //         break;
    //     default:
    //         // Use generic POSIX for unknown Unix-like systems
    //         if (configs.isUnixLikeOS()) {
    //             addExternalResolver("POSIX", std::make_unique<PosixResolver>());
    //         }
    //         break;
    // }
    
    // Add plugin-based resolvers if specified
    for (const auto& plugin : configs.plugins) {
        // This would load and initialize plugin-based resolvers
        // Implementation depends on your plugin system
        // loadPluginResolver(plugin);
    }
}

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

void IRGenerator::optimizeModule(int level) {
    if (level == -1) {
        DEBUG_LOG("No optimization taking place");
        return;
    }
    
    DEBUG_LOG("Running module verification before optimization...");

    if (llvm::verifyModule(*Module, &llvm::errs())) {
        console.error("Module verification failed before optimization");
    }

    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager cam;
    llvm::ModuleAnalysisManager mam;

    llvm::PassBuilder pb;

    // Register analyses with PassBuilder
    pb.registerModuleAnalyses(mam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.registerCGSCCAnalyses(cam);

    // Link all the analysis managers together
    pb.crossRegisterProxies(lam, fam, cam, mam);

    // Choose optimization level
    llvm::OptimizationLevel optLevel = llvm::OptimizationLevel::O2;
    if (level == 0) optLevel = llvm::OptimizationLevel::O0;
    else if (level == 1) optLevel = llvm::OptimizationLevel::O1;
    else if (level == 2) optLevel = llvm::OptimizationLevel::O2;
    else if (level >= 3) optLevel = llvm::OptimizationLevel::O3;

    // Build pipeline
    llvm::ModulePassManager mpm = pb.buildPerModuleDefaultPipeline(optLevel);

    if (!Module) {
        console.error("Module is null before optimization");
    }

    // Run pipeline
     
    mpm.run(*Module, mam);
}

bool IRGenerator::symbolExistsInStaticLib(const std::string& libPath, const std::string& symbolName) {
    std::string command = "llvm-nm \"" + libPath + "\" 2>&1";

    std::array<char, 512> buffer;
    std::string output;

    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        console.error("Failed to run llvm-nm");
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }

    _pclose(pipe);

    return output.find(symbolName) != std::string::npos;
}

void IRGenerator::compileAllFunctionBodies(SymbolTableType scope) {
    for (const auto& func : userDefinedFunctions) {
        DEBUG_LOG("Generating body for function: " + func->name + " (mangled: " + func->mangledName + ")");
        auto llvmFunc = currentModule->getFunction(func->mangledName);
        if (!llvmFunc) {
            console.error("Function not found in module during body generation: " + func->mangledName);
            continue;
        }

        generateFunctionBody(
            func->mangledName,
            llvmFunc,
            func->parameters,
            func->body,
            scope
        );
    }
}

llvm::Value* IRGenerator::codegen(std::shared_ptr<Omniscript::Expression> value, SymbolTableType scope) {
    DEBUG_LOG();
    if (!value) {
        console.error("There is no value to be codegened.");
    }
    if (!scope) {
        console.error("There is no scope for codegen to perform its operations in.");
    }

    DEBUG_LOG("Calling codegen on scope '" + scope->getName() + "' for '" + value->toString() + "'.");
    
    llvm::Value* result = codegenPrimitive(value, scope);

    if (result) {
        return result;
    }

    // Handle VariableAssignment
    if (auto varAssign = std::dynamic_pointer_cast<Omniscript::VariableAssignment>(value)) {
        DEBUG_LOG("Assigning variable " + varAssign->variableName + " of type " + varAssign->getType()->toString());
        return assignVariable(varAssign, scope);
    }

    if (auto castExpr = std::dynamic_pointer_cast<Omniscript::CastExpression>(value)) {
        DEBUG_LOG("Generating cast from type " + castExpr->targetExpr->getType()->toString() + " to " + castExpr->type->toString());

        llvm::Value* src = codegen(castExpr->targetExpr, scope);
        if (!src) return nullptr;

        llvm::Type* destType = resolveLLVMType(castExpr->type);
        return generateCast(src, destType);
    }

    if (auto nullable = std::dynamic_pointer_cast<Omniscript::NullableExpression>(value)) {
        DEBUG_LOG("Generating nullable expression");

        if (nullable->isNull()) {
            DEBUG_LOG("Nullable expression has null value");
            // Return a 'null' representation
            // Assuming you use `{ i1, T }` style struct, set `is_null = true` and `value = undef`
            llvm::Type* innerType = resolveLLVMType(nullable->getType()); // Get full nullable type
            llvm::StructType* nullableType = llvm::StructType::get(*Context, {
                llvm::Type::getInt1Ty(*Context), // is_null
                innerType
            });

            llvm::Value* undefVal = llvm::UndefValue::get(innerType);
            llvm::Value* isNull = llvm::ConstantInt::getFalse(*Context);

            llvm::Value* result = llvm::UndefValue::get(nullableType);
            result = Builder->CreateInsertValue(result, isNull, {0});
            result = Builder->CreateInsertValue(result, undefVal, {1});
            return result;
        }

        // If value is not null
        // Generate the value
        llvm::Value* innerValue = codegen(nullable->inner, scope);

        llvm::Type* innerType = innerValue->getType();
        llvm::StructType* nullableType = llvm::StructType::get(*Context, {
            llvm::Type::getInt1Ty(*Context), // is_null
            innerType
        });

        llvm::Value* isNull = llvm::ConstantInt::getTrue(*Context);

        llvm::Value* result = llvm::UndefValue::get(nullableType);
        result = Builder->CreateInsertValue(result, isNull, {0});
        result = Builder->CreateInsertValue(result, innerValue, {1});

        return result;
    }

    // Handle ReferenceValue
    if (auto refValue = std::dynamic_pointer_cast<Omniscript::ReferenceExpression>(value)) {
        DEBUG_LOG("Creating reference to variable " + refValue->referentName);
        return getReferenceToVariable(refValue->referentName);
    }
    
    if (auto addressOf = std::dynamic_pointer_cast<Omniscript::AddressOfExpression>(value)) {
        DEBUG_LOG("Getting the address of variable " + addressOf->variableName);
        return getAddressOf(addressOf->variableName);
    }

    if (auto null = std::dynamic_pointer_cast<Omniscript::NullExpression>(value)) {
        DEBUG_LOG("Creating a null value");
        return createNullValue();
    }

    else if (auto rawPtr = std::dynamic_pointer_cast<Omniscript::RawPointerExpression>(value)) {
        DEBUG_LOG("Creating raw pointer from address: " + std::to_string(rawPtr->address));
        
        // Get the LLVM type for the pointee
        llvm::Type* pointeeType = resolveLLVMType(rawPtr->getType()->getPointeeType());
        
        // Handle null pointer case
        if (rawPtr->address == 0) {
            return llvm::ConstantPointerNull::get(
                llvm::PointerType::get(pointeeType, 0)
            );
        }
        
        // Create the raw pointer
        return createRawPointer(rawPtr->address, pointeeType);
    }
    
    if (auto block = std::dynamic_pointer_cast<Omniscript::BlockExpression>(value)) {
        DEBUG_LOG("Evaluating a block value — First pass (registration)");

        for (const auto& expr : block->values) {
            if (auto func = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(expr)) {
                DEBUG_LOG("Processing function declaration: " + func->name + " (mangled: " + func->mangledName + ")");
                llvm::Type* returnType = resolveLLVMType(func->returnType);

                if (func->isExtern) {
                    createExternFunction(func, scope);
                } else if (func->isIntrinsic) {
                    std::string nameWithoutPrefix = func->intrinsicName;
                    const std::string prefix = "intrinsic_";
                    if (nameWithoutPrefix.rfind(prefix, 0) == 0)
                        nameWithoutPrefix = nameWithoutPrefix.substr(prefix.size());

                    createIntrinsicFunction(
                        func->mangledName,
                        "llvm." + nameWithoutPrefix,
                        returnType
                    );
                } else {
                    registerFunction(
                        func->mangledName,
                        returnType,
                        func->parameters,
                        scope,
                        func->isVarArg
                    );
                    userDefinedFunctions.push_back(func);
                }
            }
        }

        for (const auto& expr : block->values) {
            if (std::dynamic_pointer_cast<Omniscript::FunctionExpression>(expr)) {
                continue;
            }

            if (auto ret = std::dynamic_pointer_cast<Omniscript::ReturnExpression>(expr)) {
                if (ret->getType()) {
                    if (!ret->getType()->isVoidLike()) {
                        return codegen(expr, scope);
                    }
                } else {
                    console.error("The return type has no type");
                }
            } 

            if (auto varAssign = std::dynamic_pointer_cast<Omniscript::VariableAssignment>(expr)) {
                if (!varAssign->isStatic) {
                    varAssign->isGlobal = block->isGlobal;
                }
            }

            codegen(expr, scope);
        }

        return nullptr;
    }

    if (auto nullpointer = std::dynamic_pointer_cast<Omniscript::NullPointerExpression>(value)) {
        DEBUG_LOG("Creating a null pointer of type " + nullpointer->getType()->toString());
        DEBUG_LOG("Creating a null pointer of root type " + nullpointer->getRootType()->toString());
        auto pointeeType = resolveLLVMType(nullpointer->getRootType()->getPointeeType());
        return createNullPointer(pointeeType);
    }

    if (auto func = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(value)) {
        DEBUG_LOG("Creating an overload for function " + func->name + " with mangled name '" + func->mangledName + "'");
        llvm::Type* returnType = resolveLLVMType(func->returnType);
        if (func->isExtern) {
            return createExternFunction(func, scope);
        } else if (func->isIntrinsic) {
            std::string nameWithoutPrefix = func->intrinsicName;
            const std::string prefix = "intrinsic_";

            // Strip 'intrinsic_' prefix if present
            if (nameWithoutPrefix.rfind(prefix, 0) == 0) {
                nameWithoutPrefix = nameWithoutPrefix.substr(prefix.size());
            }

            return createIntrinsicFunction(
                func->mangledName,
                "llvm." + nameWithoutPrefix,
                returnType
            );
        }
        registerFunction(
            func->mangledName,
            returnType,
            func->parameters,
            scope,
            func->isVarArg
        );
        userDefinedFunctions.push_back(func);
    }

    if (auto ret = std::dynamic_pointer_cast<Omniscript::ReturnExpression>(value)) {
        DEBUG_LOG("Creating a return statement of kind '" + ret->getType()->toString() + "'.");

        llvm::Type* type = resolveLLVMType(ret->getType());
        if (!ret->value) {
            return createReturn(nullptr, nullptr);
        }
        llvm::Value* val = codegen(ret->value, scope);
        return createReturn(val, type);
    }

    if (auto unary = std::dynamic_pointer_cast<Omniscript::UnaryExpression>(value)) {
        DEBUG_LOG("Creating a unary expression");
        llvm::Value* operandVal = codegen(unary->operand, scope);
        if (!operandVal) {
            console.error("The operand value is invalid");
        }
        return createUnaryExpression(operandVal, unary->op.getType(), unary->isPrefix);
    }

    if (auto binary = std::dynamic_pointer_cast<Omniscript::BinaryExpression>(value)) {
        DEBUG_LOG("Creating a binary expression: " + binary->left->toString() + " " + binary->op.getValue() + " " + binary->right->toString());
        llvm::Value* lhs = codegen(binary->left, scope);
        llvm::Value* rhs = codegen(binary->right, scope);
        DEBUG_LOG("lhs type: '" + debugType(lhs->getType()) + "' rhs type: '" + debugType(rhs->getType()) + "'.");
        if (!lhs) {
            console.error("The lhs value is invalid");
        }
        if (!rhs) {
            console.error("The rhs value is invalid");
        }
        return createBinaryExpression(lhs, binary->op.getType(), rhs);
    }

    if (auto ternary = std::dynamic_pointer_cast<Omniscript::TernaryExpression>(value)) {
        DEBUG_LOG("Creating a ternary expression");
        llvm::Value* cond = codegen(ternary->condition, scope);
        llvm::Value* truthy = codegen(ternary->truthy, scope);
        llvm::Value* falsey = codegen(ternary->falsey, scope);
        if (!cond || !truthy || !falsey) return nullptr;
        return createTernaryExpression(cond, truthy, falsey);
    }

    if (auto var = std::dynamic_pointer_cast<Omniscript::VariableAccessExpression>(value)) {
        DEBUG_LOG("Accessing variable: " + var->variableName);
        if (var->extractValue) {
            DEBUG_LOG("Extracting the value of a nullable");
        }
        // return getVariable(var->variableName, var->extractValue);
        return getVariable(var->variableName, false);
    }

    if (auto call = std::dynamic_pointer_cast<Omniscript::CallExpression>(value)) {
        DEBUG_LOG("Calling " + call->calleeName);
        
        std::vector<llvm::Value*> args;
        args.reserve(call->args.size()); // Pre-reserve to avoid reallocations

        for (const auto& arg : call->args) {
            DEBUG_LOG(arg->toString());

            if (auto arr = std::dynamic_pointer_cast<Omniscript::ArrayExpression>(arg); arr && arr->isVariadicArray) {
                // If variadic, reserve space ahead (optional perf tweak)
                args.reserve(args.size() + arr->elements.size());

                for (const auto& element : arr->elements) {
                    if (auto value = codegen(element, scope)) {
                        args.push_back(value);
                    } else {
                        console.error(formatError("Failed to generate code for variadic argument element."));
                    }
                }
            } else {
                if (auto value = codegen(arg, scope)) {
                    args.push_back(value);
                } else {
                    console.error(formatError("Failed to generate code for argument: " + arg->toString()));
                }
            }
        }


        if (call->instanceName.empty()) {
            DEBUG_LOG("Creating a normal call for " + call->calleeName);
            return createCall(call->calleeName, args);
        }
        DEBUG_LOG("Creating an object instance");
        return createObjectInstance(call->calleeName, call->instanceName, args, call->isGlobal);
    }

    if (auto structExpr = std::dynamic_pointer_cast<Omniscript::StructExpression>(value)) {
        std::vector<llvm::Type*> fieldTypes;
        fieldTypes.reserve(structExpr->parameters.size());
        
        int methodsCount = structExpr->parameters.size();

        for (const auto& field : structExpr->parameters) {
            if (auto input = std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(field)) {
                llvm::Type* llvmFieldType = resolveLLVMType(input->getType());
                if (!llvmFieldType) {
                    console.error("Failed to generate type for field '" + input->name + "' in struct '" + structExpr->name + "'.");
                    return nullptr;
                }
        
                fieldTypes.push_back(llvmFieldType);
                methodsCount--;
            }
        }
        
        // Create the LLVM struct type (opaque or packed depending on your system)
        createStructType(structExpr->name, fieldTypes);

        if (methodsCount > 0) {
            std::vector<llvm::Function*> methods;
    
            methods.reserve(methodsCount);
    
            for (const auto& field : structExpr->parameters) {
                if (auto method = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(field)) {
                    llvm::Type* returnType = resolveLLVMType(method->returnType);
                    llvm::Function* methd = registerFunction(method->mangledName, returnType, method->parameters, scope);
                    methods.emplace_back(methd);
                }
            }
            
            int methodIndex = 0;
            for (const auto& field : structExpr->parameters) {
                if (auto method = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(field)) {
                    generateFunctionBody(
                        method->mangledName,
                        methods[methodIndex],
                        method->parameters,
                        method->body,
                        scope
                    );
                    methodIndex++;
                }
        }
        }

        return nullptr;
    }

    if (auto classExpr = std::dynamic_pointer_cast<Omniscript::ClassExpression>(value)) {
        return codegen(classExpr->structExpr, scope);
    }

    if (auto ifExpr = std::dynamic_pointer_cast<Omniscript::IfExpression>(value)) {
        DEBUG_LOG("Creating an if expression");
        return createIfStatement(ifExpr->conditions, ifExpr->bodies, ifExpr->elseBody, scope);
    }

    if (auto enumExpr = std::dynamic_pointer_cast<Omniscript::EnumExpression>(value)) {
        DEBUG_LOG("Processing EnumExpression for enum '" + enumExpr->enumName + "'");
    
        std::vector<std::string> names;
        std::vector<llvm::Value*> values;
    
        // Reserve space to avoid repeated allocations
        names.reserve(enumExpr->expressionMap.size());
        values.reserve(enumExpr->expressionMap.size());
    
        for (const auto& [name, val] : enumExpr->expressionMap) {
            llvm::Value* enumValue = codegen(val, scope); // Generate LLVM IR for each enum entry
    
            if (enumValue) {
                names.emplace_back(name);
                values.emplace_back(enumValue);
                DEBUG_LOG("Enum '" + enumExpr->enumName + "' has enumerator '" + name + "' with value " + val->toString());
            } else {
                DEBUG_LOG("Failed to generate IR for enumerator '" + name + "'");
            }
        }
    
        // Call the appropriate method depending on flags
        if (enumExpr->isEnumClass && enumExpr->hasLookup) {
            return createEnumClassWithLookup(names, values, enumExpr->enumName, /*isGlobal=*/true);
        } else if (enumExpr->isEnumClass) {
            return createEnumClass(names, values, enumExpr->enumName, /*isGlobal=*/true);
        } else if (enumExpr->hasLookup) {
            return createEnumWithLookup(names, values, enumExpr->enumName, /*isGlobal=*/true);
        } else {
            return createEnum(names, values, enumExpr->enumName, /*isGlobal=*/true);
        }
    }
    
    if (auto forExpr = std::dynamic_pointer_cast<Omniscript::ForLoopExpression>(value)) {
        DEBUG_LOG("Processing ForLoopExpression");
        return createForLoop(forExpr, scope);
    }

    if (auto whileExpr = std::dynamic_pointer_cast<Omniscript::WhileLoopExpression>(value)) {
        DEBUG_LOG("Processing WhileLoopExpression");
        return createWhileLoop(whileExpr, scope);
    }

    // Handle access expressions recursively
    if (auto accessExpr = std::dynamic_pointer_cast<Omniscript::AccessExpression>(value)) {
        return handleAccessExpression(accessExpr, scope);
    }

    if (auto moduleExpr = std::dynamic_pointer_cast<Omniscript::ModuleExpression>(value)) {
        DEBUG_LOG("Processing ModuleExpression: " + moduleExpr->name);
    
        // Create a new scope for the module
        auto moduleScope = std::make_shared<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>>(scope);
    
        // Store generated IR values for module members
        std::unordered_map<std::string, llvm::Value*> memberIRValues;
    
        for (const auto& member : moduleExpr->members) {
            std::string memberName = member->name;
            DEBUG_LOG("Generating IR for module member: " + memberName);
    
            llvm::Value* memberValue = codegen(member->value, moduleScope);
            if (!memberValue) {
                console.error("Failed to generate IR for module member: " + memberName);
                continue;
            }
    
            // Save the member value and type
            memberIRValues[memberName] = memberValue;
            // moduleScope->define(memberName, memberExpr->getType());
        }
    
        // Generate the actual module object
        llvm::Value* moduleInstance = createModuleObject(
            moduleExpr->name,
            memberIRValues
        );
    
        return moduleInstance;
    }   

    return nullptr;
}

void IRGenerator::scheduleGlobalInitialization(
    const std::string& name,
    llvm::GlobalVariable* gVar,
    llvm::Value* initialValue
) {
    // Create initialization in global constructor
    llvm::IRBuilder<> initBuilder(
        Module->getContext()
    );
    auto* initFunc = getOrCreateGlobalInitFunction();
    auto* entry = &initFunc->getEntryBlock();
    
    if (entry->empty()) {
        initBuilder.SetInsertPoint(entry);
    } else {
        initBuilder.SetInsertPoint(
            entry, 
            std::prev(entry->end())
        );
    }

    // Store the initial value
    initBuilder.CreateStore(initialValue, gVar);
}

void IRGenerator::finalizeGlobalInitializers() {
    if (globalInitializers.empty()) return;

    auto* func = getOrCreateGlobalInitFunction();
    auto* entry = &func->getEntryBlock();
    
    // Set insertion point at end of entry block
    Builder->SetInsertPoint(entry, entry->end());

    // Emit all initializers
    for (auto& init : globalInitializers) {
        if (init.value->getType() != init.variable->getValueType()) {
            init.value = Builder->CreateBitCast(init.value, init.variable->getValueType());
        }
        Builder->CreateStore(init.value, init.variable);
    }

    globalInitializers.clear();
}

bool IRGenerator::currentBlockHasTerminator() const {
    return Builder->GetInsertBlock()->getTerminator() != nullptr;
}

bool IRGenerator::isNullableStruct(llvm::Type* type) {
    // if (auto* structType = llvm::dyn_cast<llvm::StructType>(type)) {
    //     return structType->getNumElements() == 2 &&
    //            structType->getElementType(0)->isIntegerTy(1);  // i1
    // }
    return false;
}
