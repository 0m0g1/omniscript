#include <omniscript/utils.h>
#include <omniscript/engine/Lexer.h>
#include <omniscript/engine/Parser.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/Core/Target_config.h>
#include <omniscript/engine/JITCompiler.h>
#include <omniscript/engine/EngineConfigs.h>
#include <omniscript/engine/Backends/JITBackend.h>
#include <omniscript/engine/Backends/llvm/LLVMJITBackend.h>
#include <omniscript/engine/Backends/llvm/LLVMAOTBackend.h>

class Compiler {
public:
    void compile(const std::vector<std::shared_ptr<Statement>>& statements, const Config &config) {
        DEBUG_LOG("Compiling source code with AOT backend...");
        
        // Print target information if in debug mode
        if (config.diagnostics.debugMode) {
            printTargetInfo(config);
        }
        
        // Validate target compatibility
        if (!validateTargetConfiguration(config)) {
            console.error("Invalid target configuration. Compilation aborted.");
            return;
        }
        
        auto backend = std::make_shared<LLVMAOTBackend>();
        backend->initialize();
        backend->execute(statements, config);
        console.log("AOT Compilation completed.");
    }
    
private:
     // Enhanced printTargetInfo using Config methods:
    static void printTargetInfo(const Config& config) {
        DEBUG_LOG("=== Target Information ===");
        DEBUG_LOG("Target Triple: " + config.getEffectiveTargetTriple());
        DEBUG_LOG("Target Summary: " + config.getTargetSummary());
        DEBUG_LOG("Architecture: " + config.getArchitectureName());
        DEBUG_LOG("Pointer Size: " + std::to_string(config.getPointerSize()) + " bytes");
        DEBUG_LOG("64-bit: " + std::string(config.isArchitecture64Bit() ? "yes" : "no"));
        DEBUG_LOG("Operating System: " + config.getOSName());
        DEBUG_LOG("Executable Extension: " + config.getExecutableExtension());
        DEBUG_LOG("Cross Compilation: " + std::string(config.isCrossCompilation() ? "yes" : "no"));
        
        // Print available features
        auto features = config.getAvailableFeatures();
        if (!features.empty()) {
            std::string featuresStr = "Available Features: ";
            for (const auto& feature : features) {
                featuresStr += feature + " ";
            }
            DEBUG_LOG(featuresStr);
        }
        
        DEBUG_LOG("Default CPU: " + config.getDefaultCPU());
        DEBUG_LOG("Unix-like OS: " + std::string(config.isUnixLikeOS() ? "yes" : "no"));
    }
    
    // Enhanced validation using Config methods:
    static bool validateTargetConfiguration(const Config& config) {
        // The Config class now has comprehensive validation
        std::string errorMessage;
        if (!config.validate(errorMessage)) {
            console.error("Target validation failed: " + errorMessage);
            return false;
        }
        
        // Additional custom validation if needed
        if (config.isJITMode() && config.isCrossCompilation()) {
            console.error("JIT mode cannot be used with cross-compilation");
            return false;
        }
        
        return true;
    }
};

class Engine {
public:
    static constexpr const char* VERSION = "1.0.0";

    static void printUsage() {
        console.log("Usage: omniscript [options] <file>");
        console.log("Options:");
        console.log("  --debug, -d              Enable debug mode");
        console.log("  --dry                    Compile to object file only");
        console.log("  --entry <function>       The function to call when starting the program");
        console.log("  --emit-staticlib         Emit a static library");
        console.log("  --emit-sharedlib         Emit a shared library");
        console.log("  --emit-assembly          Emit assembly source");
        console.log("  --emit-ir                Emit LLVM IR");
        console.log("  --execute                Execute statements using JIT");
        console.log("  --keep-obj               Keep intermediate object files");
        console.log("  --log-asm                Log the final generated assembly code");
        console.log("  --log-final-code         Log the final generated IR code");
        console.log("  --make                   Compile the source code (AOT)");
        console.log("  --output, -o <file>      Set output file path for AOT");
        console.log("  --optimization-level, -O <n> Optimization level: 0 (none) to 3 (max)");
        console.log("  --target-arch <arch>     Target architecture:");
        console.log("                           x86_64, arm64, x86_32, arm32, riscv64, wasm32, wasm64, auto");
        console.log("  --target-os <os>         Target operating system:");
        console.log("                           linux, windows, macos, freebsd, android, ios, wasm, auto");
        console.log("  --target-triple <triple> Set target triple directly (overrides --target-arch/--target-os)");
        console.log("  --list-targets           List available target architectures and operating systems");
        console.log("  --show-host-info         Show information about the host system");
        console.log("  --gc <strategy>          Garbage collection strategy (none, refcounting, marksweep, etc.)");
        console.log("  --safety <level>         Safety level (unsafe, minimal, standard, paranoid)");
        console.log("  --enable-lto             Enable Link Time Optimization");
        console.log("  --enable-pgo             Enable Profile Guided Optimization");
        console.log("  --version                Display version information");
        console.log("  --help                   Display this help message");
        console.log("  --verbose                Show full parse tree and token stream");
    }

    static void listTargets() {
        console.log("Available Target Architectures:");
        console.log("  x86_64  - 64-bit x86 (Intel/AMD)");
        console.log("  arm64   - 64-bit ARM (Apple Silicon, etc.)");
        console.log("  x86_32  - 32-bit x86");
        console.log("  arm32   - 32-bit ARM");
        console.log("  riscv64 - 64-bit RISC-V");
        console.log("  wasm32  - 32-bit WebAssembly");
        console.log("  wasm64  - 64-bit WebAssembly");
        console.log("  auto    - Auto-detect (default)");
        console.log("");
        console.log("Available Target Operating Systems:");
        console.log("  linux   - Linux");
        console.log("  windows - Windows");
        console.log("  macos   - macOS");
        console.log("  freebsd - FreeBSD");
        console.log("  android - Android");
        console.log("  ios     - iOS");
        console.log("  wasm    - WebAssembly");
        console.log("  auto    - Auto-detect (default)");
        console.log("");
        console.log("Examples:");
        console.log("  --target-arch x86_64 --target-os linux");
        console.log("  --target-triple x86_64-unknown-linux-gnu");
        console.log("  --target-arch arm64 --target-os macos");
    }

    static TargetArch parseTargetArch(const std::string& arch) {
        if (arch == "x86_64") return TargetArch::X86_64;
        if (arch == "arm64" || arch == "aarch64") return TargetArch::ARM64;
        if (arch == "x86_32" || arch == "i386") return TargetArch::X86_32;
        if (arch == "arm32" || arch == "arm") return TargetArch::ARM32;
        if (arch == "riscv64") return TargetArch::RISCV64;
        if (arch == "wasm32") return TargetArch::WASM32;
        if (arch == "wasm64") return TargetArch::WASM64;
        if (arch == "auto") return TargetArch::Auto;
        
        console.warn("Unknown architecture '" + arch + "', using auto-detection");
        return TargetArch::Auto;
    }

    static TargetOS parseTargetOS(const std::string& os) {
        if (os == "linux") return TargetOS::Linux;
        if (os == "windows" || os == "win32") return TargetOS::Windows;
        if (os == "macos" || os == "darwin") return TargetOS::MacOS;
        if (os == "freebsd") return TargetOS::FreeBSD;
        if (os == "android") return TargetOS::Android;
        if (os == "ios") return TargetOS::iOS;
        if (os == "wasm" || os == "webassembly") return TargetOS::WebAssembly;
        if (os == "auto") return TargetOS::Auto;
        
        console.warn("Unknown operating system '" + os + "', using auto-detection");
        return TargetOS::Auto;
    }

    static std::pair<TargetArch, TargetOS> parseTargetTriple(const std::string& triple) {
        auto parsed = TargetInfo::TargetTriple::parse(triple);
        
        // Map common LLVM arch names to our enum
        TargetArch arch = TargetArch::Auto;
        if (parsed.arch == "x86_64") arch = TargetArch::X86_64;
        else if (parsed.arch == "aarch64") arch = TargetArch::ARM64;
        else if (parsed.arch == "i386" || parsed.arch == "i686") arch = TargetArch::X86_32;
        else if (parsed.arch == "arm") arch = TargetArch::ARM32;
        else if (parsed.arch == "riscv64") arch = TargetArch::RISCV64;
        else if (parsed.arch == "wasm32") arch = TargetArch::WASM32;
        else if (parsed.arch == "wasm64") arch = TargetArch::WASM64;
        
        // Map common OS names to our enum
        TargetOS os = TargetOS::Auto;
        if (parsed.os == "linux") os = TargetOS::Linux;
        else if (parsed.os == "windows") os = TargetOS::Windows;
        else if (parsed.os == "darwin") os = TargetOS::MacOS;
        else if (parsed.os == "freebsd") os = TargetOS::FreeBSD;
        else if (parsed.os == "android") os = TargetOS::Android;
        else if (parsed.os == "ios") os = TargetOS::iOS;
        else if (parsed.os == "wasm") os = TargetOS::WebAssembly;
        
        return {arch, os};
    }

    static GCStrategy parseGCStrategy(const std::string& gc) {
        if (gc == "none") return GCStrategy::None;
        if (gc == "refcounting") return GCStrategy::RefCounting;
        if (gc == "marksweep") return GCStrategy::MarkSweep;
        if (gc == "generational") return GCStrategy::Generational;
        if (gc == "incremental") return GCStrategy::Incremental;
        return GCStrategy::RefCounting; // Default
    }

    static SafetyLevel parseSafetyLevel(const std::string& safety) {
        if (safety == "unsafe") return SafetyLevel::Unsafe;
        if (safety == "minimal") return SafetyLevel::Minimal;
        if (safety == "standard") return SafetyLevel::Standard;
        if (safety == "paranoid") return SafetyLevel::Paranoid;
        return SafetyLevel::Standard; // Default
    }

    static Config parseArguments(int argc, char* argv[]) {
        Config config;
        bool fileSpecified = false;

        // Set default targets to auto-detect
        config.targetArch = TargetArch::Auto;
        config.targetOS = TargetOS::Auto;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--debug" || arg == "-d") {
                config.diagnostics.debugMode = true;
                console.enableDebug();
            } else if (arg == "--verbose") {
                config.diagnostics.verbose = true;
            } else if (arg == "--execute") {
                config.mode = CompileMode::JIT;
            } else if (arg == "--make") {
                config.mode = CompileMode::AOT;
            } else if (arg == "--dry") {
                config.mode = CompileMode::DryCompile;
            } else if (arg == "--version") {
                console.log(std::string("Omniscript version ") + VERSION);
                std::exit(0);
            } else if (arg == "--help") {
                printUsage();
                std::exit(0);
            } else if (arg == "--list-targets") {
                listTargets();
                std::exit(0);
            } else if (arg == "--show-host-info") {
                TargetInfo::printHostInfo();
                std::exit(0);
            } else if (arg == "--entry") {
                if (i + 1 < argc) {
                    config.entry = argv[++i];
                } else {
                    console.error("Error: Missing function name after '--entry'.");
                }
            } else if (arg == "--log-asm") {
                config.diagnostics.logAsm = true;
            } else if (arg == "--log-final-code") {
                config.diagnostics.logFinalCode = true;
            } else if (arg == "--optimization-level" || arg == "-O") {
                if (i + 1 < argc) {
                    config.optimization.level = std::stoi(argv[++i]);
                } else {
                    console.error("Error: Missing integer after '--optimization-level'.");
                }
            } else if (arg == "--output" || arg == "-o") {
                if (i + 1 < argc) {
                    config.outputPath = argv[++i];
                } else {
                    console.error("Error: Missing path after '--output'.");
                }
            } else if (arg == "--keep-obj") {
                config.keepIntermediateFiles = true;
            } else if (arg == "--show-metadata") {
                config.diagnostics.showMetadata = true;
            } else if (arg == "--emit-staticlib") {
                config.mode = CompileMode::AOT;
                config.aot.outputFormat = OutputFormat::StaticLib;
            } else if (arg == "--emit-sharedlib") {
                config.mode = CompileMode::AOT;
                config.aot.outputFormat = OutputFormat::SharedLib;
            } else if (arg == "--emit-assembly") {
                config.mode = CompileMode::AOT;
                config.aot.outputFormat = OutputFormat::Assembly;
            } else if (arg == "--emit-ir") {
                config.mode = CompileMode::AOT;
                config.aot.outputFormat = OutputFormat::LLVM_IR;
            } else if (arg == "--target-arch") {
                if (i + 1 < argc) {
                    config.targetArch = parseTargetArch(argv[++i]);
                } else {
                    console.error("Error: Missing architecture after '--target-arch'.");
                }
            } else if (arg == "--target-os") {
                if (i + 1 < argc) {
                    config.targetOS = parseTargetOS(argv[++i]);
                } else {
                    console.error("Error: Missing OS after '--target-os'.");
                }
            } else if (arg == "--target-triple") {
                if (i + 1 < argc) {
                    auto [arch, os] = parseTargetTriple(argv[++i]);
                    config.targetArch = arch;
                    config.targetOS = os;
                } else {
                    console.error("Error: Missing triple after '--target-triple'.");
                }
            } else if (arg == "--gc") {
                if (i + 1 < argc) {
                    config.runtime.gcStrategy = parseGCStrategy(argv[++i]);
                } else {
                    console.error("Error: Missing GC strategy after '--gc'.");
                }
            } else if (arg == "--safety") {
                if (i + 1 < argc) {
                    config.runtime.safetyLevel = parseSafetyLevel(argv[++i]);
                } else {
                    console.error("Error: Missing safety level after '--safety'.");
                }
            } else if (arg == "--enable-lto") {
                config.aot.lto.enabled = true;
            } else if (arg == "--enable-pgo") {
                config.optimization.pgo.enabled = true;
            } else {
                if (!fileSpecified) {
                    config.filePath = arg;
                    fileSpecified = true;
                } else {
                    console.warn("Multiple file paths provided. Only the first will be used.");
                }
            }
        }

        if (!fileSpecified) {
            console.error("Error: File path is required. Use '-' to read from stdin.");
        }

        if (config.mode == CompileMode::None) {
            config.mode = CompileMode::JIT; // Default
        }

        // Resolve auto-detected targets
        resolveTargetConfiguration(config);

        // Set default output path if not specified
        if (config.outputPath.empty() && config.isAOTMode()) {
            setDefaultOutputPath(config);
        }

        // Auto-configure all subsystems based on target
        config.autoConfigureForTarget();

        // Validate configuration
        std::string errorMessage;
        if (!config.validate(errorMessage)) {
            console.error("Configuration validation failed: " + errorMessage);
            std::exit(1);
        }

        return config;
    }

    static void resolveTargetConfiguration(Config& config) {
        // Auto-detect architecture if needed
        if (config.targetArch == TargetArch::Auto) {
            config.targetArch = TargetInfo::detectHostArchitecture();
            DEBUG_LOG("Auto-detected target architecture: " + TargetInfo::getArchitectureName(config.targetArch));
        }
        
        // Auto-detect OS if needed
        if (config.targetOS == TargetOS::Auto) {
            config.targetOS = TargetInfo::detectHostOS();
            DEBUG_LOG("Auto-detected target OS: " + TargetInfo::getOSName(config.targetOS));
        }
    }

    // Use Config's built-in output path handling:
    static void setDefaultOutputPath(Config& config) {
        if (config.outputPath.empty() || config.outputPath == "a.out") {
            std::string baseName = config.filePath;
            
            // Remove extension if present
            size_t lastDot = baseName.find_last_of('.');
            if (lastDot != std::string::npos) {
                baseName = baseName.substr(0, lastDot);
            }
            
            // Use Config's methods for extensions
            std::string extension;
            switch (config.aot.outputFormat) {
                case OutputFormat::Executable:
                    extension = config.getExecutableExtension();
                    break;
                case OutputFormat::SharedLib:
                    extension = config.getSharedLibExtension();
                    break;
                case OutputFormat::StaticLib:
                    extension = config.getStaticLibExtension();
                    break;
                case OutputFormat::Assembly:
                    extension = ".s";
                    break;
                case OutputFormat::LLVM_IR:
                    extension = ".ll";
                    break;
                case OutputFormat::ObjectFile:
                    extension = config.getObjectFileExtension();
                    break;
            }
            
            config.outputPath = baseName + extension;
            DEBUG_LOG("Default output path set to: " + config.outputPath);
        }
    }

    static std::string readSourceCode(const Config& config) {
        if (config.filePath == "-") {
            std::stringstream buffer;
            buffer << std::cin.rdbuf();
            return buffer.str();
        } else {
            std::ifstream file(config.filePath);
            if (!file) {
                console.error("Error opening file: " + config.filePath);
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    }

    static void printConfigDebugInfo(const Config& config) {
        DEBUG_LOG("=== Configuration Debug Info ===");
        DEBUG_LOG("File: " + config.filePath);
        DEBUG_LOG("Mode: " + std::to_string(static_cast<int>(config.mode)));
        DEBUG_LOG("Entry: " + config.entry);
        DEBUG_LOG("Output Path: " + config.outputPath);
        DEBUG_LOG("Optimization Level: " + std::to_string(config.optimization.level));
        DEBUG_LOG("Debug Mode: " + std::string(config.diagnostics.debugMode ? "true" : "false"));
        DEBUG_LOG("Log IR: " + std::string(config.diagnostics.logFinalCode ? "true" : "false"));
        DEBUG_LOG("Log ASM: " + std::string(config.diagnostics.logAsm ? "true" : "false"));
        DEBUG_LOG("Keep Obj: " + std::string(config.keepIntermediateFiles ? "true" : "false"));
        
        // Enhanced target information
        DEBUG_LOG("=== Target Configuration ===");
        DEBUG_LOG("Target Arch: " + TargetInfo::getArchitectureName(config.targetArch) + 
                  " (" + std::to_string(static_cast<int>(config.targetArch)) + ")");
        DEBUG_LOG("Target OS: " + TargetInfo::getOSName(config.targetOS) + 
                  " (" + std::to_string(static_cast<int>(config.targetOS)) + ")");
        DEBUG_LOG("Target Triple: " + TargetInfo::generateTriple(config.targetArch, config.targetOS));
        DEBUG_LOG("Cross Compilation: " + std::string(TargetInfo::isCrossCompilation(config.targetArch, config.targetOS) ? "yes" : "no"));
        
        DEBUG_LOG("=== Runtime Configuration ===");
        DEBUG_LOG("GC Strategy: " + std::to_string(static_cast<int>(config.runtime.gcStrategy)));
        DEBUG_LOG("Safety Level: " + std::to_string(static_cast<int>(config.runtime.safetyLevel)));
        
        DEBUG_LOG("=== Optimization Configuration ===");
        DEBUG_LOG("LTO Enabled: " + std::string(config.aot.lto.enabled ? "true" : "false"));
        DEBUG_LOG("PGO Enabled: " + std::string(config.optimization.pgo.enabled ? "true" : "false"));
    }

    static void run(const Config& config) {
        std::string sourceCode = readSourceCode(config);

        Lexer lexer(sourceCode, config.filePath);
        Parser parser(lexer);
        parser.setDebugMode(config.diagnostics.debugMode);
        std::vector<std::shared_ptr<Statement>> statements = parser.Parse();

        if (config.diagnostics.verbose) {
            // parser.printParseTree();
            // parser.printTokenStream();
        }

        if (config.diagnostics.debugMode) {
            printConfigDebugInfo(config);
        }

        if (config.isAOTMode()) {
            Compiler compiler;
            compiler.compile(statements, config);
            if (config.mode == CompileMode::AOT) {
                console.log("Compilation done. Output emitted to: " + config.outputPath);
            } else if (config.isDryRun()) {
                console.log("Dry compilation complete. Output written to: " + config.outputPath);
            }
        } else if (config.isJITMode()) {
            // Print target info for JIT mode too if in debug
            if (config.diagnostics.debugMode) {
                DEBUG_LOG("JIT execution using target: " + TargetInfo::getTargetSummary(config.targetArch, config.targetOS));
            }
            
            auto backend = std::make_shared<LLVMJITBackend>();
            backend->initialize();
            JITCompiler jit(backend);
            jit.execute(statements, config);
        }
    }
};

int main(int argc, char* argv[]) {
    try {
        Config config = Engine::parseArguments(argc, argv);
        Engine::run(config);
    } catch (const std::exception& e) {
        console.error(e.what());
        return 1;
    }
    return 0;
}