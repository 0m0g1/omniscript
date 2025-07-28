#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/CLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/LinuxLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/PosixLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/DarwinLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/AndroidLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/WindowsAPILLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/WebAssemblyLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/SmartPlatformLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/StaticLibraryLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/DynamicLibraryLLVMResolver.h>

namespace Omniscript {
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

void IRGenerator::setupExternalResolvers() {
    DEBUG_LOG("Setting up the external resolvers");
    // Set up external resolvers based on target OS
    auto targetOS = configs.resolveTargetOS();
    auto targetArch = configs.resolveTargetArch();
    
    DEBUG_LOG("Setting up external resolver for target: " + 
              configs.getTargetSummary());
    
    // Always add C standard library resolver
    addExternalResolver("C", std::make_unique<CStdLibResolver>());
    
    // Graphics libraries - choose based on compilation mode
    if (configs.mode == CompileMode::JIT) {
        // JIT mode - prefer dynamic libraries
        addExternalResolver("glfw", std::make_unique<DynamicLibraryResolver>("dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll"));
        addExternalResolver("gladgl", std::make_unique<DynamicLibraryResolver>("dependencies/glad/build/Windows_/lib/glad_gl.dll"));
        addExternalResolver("gladgles2", std::make_unique<DynamicLibraryResolver>("dependencies/glad/build/Windows_/lib/glad_gl.dll"));
        addExternalResolver("gladvulkan", std::make_unique<DynamicLibraryResolver>("dependencies/glad/vulkan/bin/glad_vulkan.dll"));
    } else {
        // AOT mode - prefer static libraries
        addExternalResolver("glfw", std::make_unique<StaticLibraryResolver>("dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/libglfw3.a"));
        addExternalResolver("gladgl", std::make_unique<StaticLibraryResolver>("dependencies/glad/build/Windows_/lib/libglad_gl.a"));
        addExternalResolver("gladgles2", std::make_unique<StaticLibraryResolver>("dependencies/glad/gles2/bin/libglad_gles2.a"));
        addExternalResolver("gladvulkan", std::make_unique<StaticLibraryResolver>("dependencies/glad/vulkan/bin/libglad_vulkan.a"));
    }
    
    // FFmpeg libraries - choose based on compilation mode
    if (configs.mode == CompileMode::JIT) {
        // JIT mode - use DLL import libraries
        // addExternalResolver("avfilter", std::make_unique<DynamicLibraryResolver>("dependencies/ffmpeg/bin/avfilter-11.dll"));
        // addExternalResolver("avcodec", std::make_unique<DynamicLibraryResolver>("dependencies/ffmpeg/bin/avcodec-62.dll"));
        // addExternalResolver("avformat", std::make_unique<DynamicLibraryResolver>("dependencies/ffmpeg/bin/avformat-62.dll"));
        // addExternalResolver("avutil", std::make_unique<DynamicLibraryResolver>("dependencies/ffmpeg/bin/avutil-60.dll"));
        // addExternalResolver("swscale", std::make_unique<DynamicLibraryResolver>("dependencies/ffmpeg/bin/swscale-9.dll"));
        // addExternalResolver("swresample", std::make_unique<DynamicLibraryResolver>("dependencies/ffmpeg/bin/swresample-5.dll"));
        // addExternalResolver("avdevice", std::make_unique<DynamicLibraryResolver>("dependencies/ffmpeg/bin/avdevice-62.dll"));
    } else {
        // AOT mode - use static libraries (assuming they exist)
        addExternalResolver("avfilter", std::make_unique<StaticLibraryResolver>("dependencies/ffmpeg/lib/libavfilter.a"));
        addExternalResolver("avcodec", std::make_unique<StaticLibraryResolver>("dependencies/ffmpeg/lib/libavcodec.a"));
        addExternalResolver("avformat", std::make_unique<StaticLibraryResolver>("dependencies/ffmpeg/lib/libavformat.a"));
        addExternalResolver("avutil", std::make_unique<StaticLibraryResolver>("dependencies/ffmpeg/lib/libavutil.a"));
        addExternalResolver("swscale", std::make_unique<StaticLibraryResolver>("dependencies/ffmpeg/lib/libswscale.a"));
        addExternalResolver("swresample", std::make_unique<StaticLibraryResolver>("dependencies/ffmpeg/lib/libswresample.a"));
        addExternalResolver("avdevice", std::make_unique<StaticLibraryResolver>("dependencies/ffmpeg/lib/libavdevice.a"));
    }
    
    // libcurl - choose based on compilation mode
    if (configs.mode == CompileMode::JIT) {
        addExternalResolver("curl", std::make_unique<DynamicLibraryResolver>("dependencies/libcurl/lib/dll-release-x64/libcurl.dll"));
    } else {
        addExternalResolver("curl", std::make_unique<StaticLibraryResolver>("dependencies/libcurl/lib/static-release-x64/libcurl_a.lib"));
    }
    
    // OpenAL Soft - choose based on compilation mode
    if (configs.mode == CompileMode::JIT) {
        addExternalResolver("openal", std::make_unique<DynamicLibraryResolver>("dependencies/openal-soft-1.24.3-bin/bin/Win64/soft_oal.dll"));
    } else {
        addExternalResolver("openal", std::make_unique<StaticLibraryResolver>("dependencies/openal-soft-1.24.3-bin/libs/Win64/OpenAL32.lib"));
        addExternalResolver("openal32", std::make_unique<StaticLibraryResolver>("dependencies/openal-soft-1.24.3-bin/libs/Win64/OpenAL32.lib"));
    }
    
    // OpenSSL libraries - choose based on compilation mode
    if (configs.mode == CompileMode::JIT) {
        addExternalResolver("ssl", std::make_unique<DynamicLibraryResolver>("dependencies/openssl/openssl-3.5.0/x64/bin/libssl-3-x64.dll"));
        addExternalResolver("crypto", std::make_unique<DynamicLibraryResolver>("dependencies/openssl/openssl-3.5.0/x64/bin/libcrypto-3-x64.dll"));
    } else {
        addExternalResolver("ssl", std::make_unique<StaticLibraryResolver>("dependencies/openssl/openssl-3.5.0/x64/lib/libssl.lib"));
        addExternalResolver("crypto", std::make_unique<StaticLibraryResolver>("dependencies/openssl/openssl-3.5.0/x64/lib/libcrypto.lib"));
    }
    
    // SQLite - choose based on compilation mode
    if (configs.mode == CompileMode::JIT) {
        addExternalResolver("sqlite3", std::make_unique<DynamicLibraryResolver>("dependencies/sqlite/sqlite-dll-win-x64-3500000/sqlite3.dll"));
    } else {
        addExternalResolver("sqlite3", std::make_unique<StaticLibraryResolver>("dependencies/sqlite/sqlite-lib-win-x64-3500000/sqlite3.lib"));
    }
    
    // Zlib - choose based on compilation mode
    // if (configs.mode == CompileMode::JIT) {
    //     addExternalResolver("zlib", std::make_unique<DynamicLibraryResolver>("dependencies/zlib/bin/zlib1.dll"));
    // } else {
    //     addExternalResolver("zlib", std::make_unique<StaticLibraryResolver>("dependencies/zlib/lib/zlibstatic.lib"));
    // }
    
    switch (targetOS) {
        case TargetOS::Windows: {
            // Determine system directory based on target architecture
            auto targetArch = configs.resolveTargetArch();
            std::string systemDir = (targetArch == TargetArch::X86_64) ? 
                "C:\\Windows\\System32\\" : "C:\\Windows\\SysWOW64\\";
            
            // Core Windows API libraries
            addExternalResolver("kernel32", std::make_unique<WindowsAPIResolver>(systemDir + "kernel32.dll"));
            addExternalResolver("user32",   std::make_unique<WindowsAPIResolver>(systemDir + "user32.dll"));
            addExternalResolver("gdi32",    std::make_unique<WindowsAPIResolver>(systemDir + "gdi32.dll"));
            addExternalResolver("shell32",  std::make_unique<WindowsAPIResolver>(systemDir + "shell32.dll"));
            addExternalResolver("ntdll",    std::make_unique<WindowsAPIResolver>(systemDir + "ntdll.dll"));
            addExternalResolver("msvcrt",   std::make_unique<WindowsAPIResolver>(systemDir + "msvcrt.dll"));
            
            // Network libraries
            addExternalResolver("ws2_32", std::make_unique<WindowsAPIResolver>(systemDir + "ws2_32.dll"));
            addExternalResolver("wsock32", std::make_unique<WindowsAPIResolver>(systemDir + "wsock32.dll"));
            
            // Additional Windows libraries
            addExternalResolver("advapi32", std::make_unique<WindowsAPIResolver>(systemDir + "advapi32.dll"));
            addExternalResolver("ole32", std::make_unique<WindowsAPIResolver>(systemDir + "ole32.dll"));
            addExternalResolver("oleaut32", std::make_unique<WindowsAPIResolver>(systemDir + "oleaut32.dll"));
            addExternalResolver("uuid", std::make_unique<WindowsAPIResolver>(systemDir + "rpcrt4.dll")); // UUID functions are in rpcrt4.dll
            addExternalResolver("winmm", std::make_unique<WindowsAPIResolver>(systemDir + "winmm.dll"));
            addExternalResolver("version", std::make_unique<WindowsAPIResolver>(systemDir + "version.dll"));
            addExternalResolver("bcrypt", std::make_unique<WindowsAPIResolver>(systemDir + "bcrypt.dll"));
            addExternalResolver("crypt32", std::make_unique<WindowsAPIResolver>(systemDir + "crypt32.dll"));
            
            // DirectX libraries
            addExternalResolver("d3d11", std::make_unique<WindowsAPIResolver>(systemDir + "d3d11.dll"));
            addExternalResolver("dxgi", std::make_unique<WindowsAPIResolver>(systemDir + "dxgi.dll"));
            addExternalResolver("d3dcompiler", std::make_unique<WindowsAPIResolver>(systemDir + "d3dcompiler_47.dll"));
            addExternalResolver("dsound", std::make_unique<WindowsAPIResolver>(systemDir + "dsound.dll"));
            addExternalResolver("xinput", std::make_unique<WindowsAPIResolver>(systemDir + "xinput1_4.dll"));
            
            // Math functions (Windows links to msvcrt)
            addExternalResolver("m", std::make_unique<WindowsAPIResolver>(systemDir + "msvcrt.dll"));
            
            // Additional common Windows DLLs
            addExternalResolver("comctl32", std::make_unique<WindowsAPIResolver>(systemDir + "comctl32.dll"));
            addExternalResolver("comdlg32", std::make_unique<WindowsAPIResolver>(systemDir + "comdlg32.dll"));
            addExternalResolver("imm32", std::make_unique<WindowsAPIResolver>(systemDir + "imm32.dll"));
            addExternalResolver("psapi", std::make_unique<WindowsAPIResolver>(systemDir + "psapi.dll"));
            addExternalResolver("setupapi", std::make_unique<WindowsAPIResolver>(systemDir + "setupapi.dll"));
            addExternalResolver("shlwapi", std::make_unique<WindowsAPIResolver>(systemDir + "shlwapi.dll"));
            addExternalResolver("winspool", std::make_unique<WindowsAPIResolver>(systemDir + "winspool.drv"));
            
            break;
        }   
        case TargetOS::Linux:
        case TargetOS::FreeBSD:
            addExternalResolver("libc", std::make_unique<PosixResolver>());
            addExternalResolver("libm", std::make_unique<PosixResolver>());
            addExternalResolver("libdl", std::make_unique<PosixResolver>());
            addExternalResolver("libpthread", std::make_unique<PosixResolver>());
            break;
            
        case TargetOS::MacOS:
        case TargetOS::iOS:
            addExternalResolver("libSystem", std::make_unique<DarwinResolver>());
            addExternalResolver("Foundation", std::make_unique<DarwinResolver>());
            break;
            
        case TargetOS::Android:
            addExternalResolver("libc", std::make_unique<AndroidResolver>());
            addExternalResolver("libm", std::make_unique<AndroidResolver>());
            break;
            
        case TargetOS::WebAssembly:
            addExternalResolver("wasm", std::make_unique<WebAssemblyResolver>());
            break;
            
        default:
            DEBUG_LOG("Warning: Unknown target OS, using generic resolvers only");
            break;
    }
    
    // Add user-specified library paths
    for (const auto& libPath : configs.aot.libraryPaths) {
        addExternalResolver("static_lib", 
            std::make_unique<StaticLibraryResolver>(libPath));
    }
    
    // Add user-specified libraries
    for (const auto& lib : configs.aot.libraries) {
        addExternalResolver("dynamic_lib", 
            std::make_unique<DynamicLibraryResolver>(lib));
    }
    
    DEBUG_LOG("External function resolver setup completed");

    // Add plugin-based resolvers if specified
    for (const auto& plugin : configs.plugins) {
        // This would load and initialize plugin-based resolvers
        // Implementation depends on your plugin system
        // loadPluginResolver(plugin);
    }
}

}
