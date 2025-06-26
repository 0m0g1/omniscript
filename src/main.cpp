#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Parser.h>
#include <omniscript/omniscript_pch.h>
<<<<<<< HEAD:src/main.cpp
#include <omniscript/Statement.h>
#include <omniscript/Target_config.h>
#include <omniscript/JITCompiler.h>
#include <omniscript/EngineConfigs.h>
#include <omniscript/Backends/JITBackend.h>
#include <omniscript/Backends/llvm/LLVMJITBackend.h>
#include <omniscript/Backends/llvm/LLVMAOTBackend.h>
=======
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Core/Target_config.h>
#include <omniscript/engine/JITCompiler.h>
#include <omniscript/engine/EngineConfigs.h>
#include <omniscript/engine/Backends/JITBackend.h>
#include <omniscript/engine/Backends/llvm/LLVMJITBackend.h>
#include <omniscript/engine/Backends/llvm/LLVMAOTBackend.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:src/engine/main.cpp

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
        console.log("  --show-metadata          Show metadata information");
        console.log("");
        console.log("Additional Output Formats:");
        console.log("  --emit-object            Emit object file (.o/.obj)");
        console.log("  --emit-bitcode           Emit LLVM bitcode (.bc)");
        console.log("  --emit-machine-code      Emit raw machine code");
        console.log("  --emit-wasm              Emit WebAssembly module (.wasm)");
        console.log("  --emit-ptx               Emit NVIDIA PTX assembly");
        console.log("  --emit-spirv             Emit SPIR-V for OpenCL/Vulkan");
        console.log("");
        console.log("JIT Configuration:");
        console.log("  --jit-engine <engine>    JIT engine (orc, mcjit, custom)");
        console.log("  --jit-lazy               Enable lazy compilation");
        console.log("  --jit-speculation        Enable speculative compilation");
        console.log("  --jit-threshold <n>      Compilation threshold (default: 10)");
        console.log("  --jit-tiered             Enable tiered compilation");
        console.log("  --jit-cache-size <size>  Max code cache size in MB (default: 256)");
        console.log("");
        console.log("Runtime Configuration:");
        console.log("  --heap-size <size>       Set heap size (e.g., 64MB, 1GB)");
        console.log("  --stack-size <size>      Set stack size (e.g., 8MB, 16MB)");
        console.log("  --gc-threads <n>         Number of GC threads (0 = auto)");
        console.log("  --enable-parallel-gc     Enable parallel garbage collection");
        console.log("  --enable-concurrent-gc   Enable concurrent garbage collection");
        console.log("");
        console.log("Security Options:");
        console.log("  --enable-stack-protection   Enable stack protection");
        console.log("  --enable-cfi                Enable Control Flow Integrity");
        console.log("  --enable-asan               Enable AddressSanitizer");
        console.log("  --enable-msan               Enable MemorySanitizer");
        console.log("  --enable-tsan               Enable ThreadSanitizer");
        console.log("  --enable-ubsan              Enable UndefinedBehaviorSanitizer");
        console.log("  --enable-pic                Enable Position Independent Code");
        console.log("  --enable-dep                Enable Data Execution Prevention");
        console.log("  --enable-aslr               Enable Address Space Layout Randomization");
        console.log("");
        console.log("Optimization Options:");
        console.log("  --enable-vectorization      Enable vectorization optimizations");
        console.log("  --enable-loop-unrolling     Enable loop unrolling");
        console.log("  --enable-inlining           Enable function inlining");
        console.log("  --enable-tail-calls         Enable tail call optimization");
        console.log("  --enable-dead-code-elim     Enable dead code elimination");
        console.log("  --enable-const-folding      Enable constant folding");
        console.log("  --enable-cse                Enable common subexpression elimination");
        console.log("  --enable-licm               Enable loop invariant code motion");
        console.log("  --fast-math                 Enable fast math optimizations");
        console.log("  --pgo-profile <file>        Use profile data for PGO");
        console.log("  --pgo-instrument            Instrument for profile generation");
        console.log("");
        console.log("Linking Options:");
        console.log("  --library-path <path>       Add library search path");
        console.log("  --library <lib>             Link with library");
        console.log("  --framework-path <path>     Add framework search path (macOS/iOS)");
        console.log("  --framework <fw>            Link with framework (macOS/iOS)");
        console.log("  --linker-script <script>    Use custom linker script");
        console.log("  --linker-flag <flag>        Pass flag to linker");
        console.log("  --static-linking            Use static linking");
        console.log("  --strip-symbols             Strip symbols from output");
        console.log("  --debug-info                Generate debug information");
        console.log("  --debug-format <format>     Debug info format (dwarf, codeview)");
        console.log("  --thin-lto                  Use ThinLTO instead of full LTO");
        console.log("  --lto-jobs <n>              Number of LTO parallel jobs");
        console.log("");
        console.log("Build Configuration:");
        console.log("  --temp-dir <dir>            Set temporary directory");
        console.log("  --parallel-jobs <n>         Number of parallel build jobs");
        console.log("  --enable-caching            Enable build caching");
        console.log("  --cache-dir <dir>           Set cache directory");
        console.log("  --include-path <path>       Add include search path");
        console.log("  --source-path <path>        Add source search path");
        console.log("  --import-path <path>        Add import search path");
        console.log("  --define <key>=<value>      Define preprocessor macro");
        console.log("  --undefine <key>            Undefine preprocessor macro");
        console.log("");
        console.log("Language Features:");
        console.log("  --std <standard>            Language standard (latest, v1.0, etc.)");
        console.log("  --enable-experimental       Enable experimental language features");
        console.log("  --enable-feature <feature>  Enable specific language feature");
        console.log("  --disable-feature <feature> Disable specific language feature");
        console.log("");
        console.log("Diagnostics:");
        console.log("  --log-optimization          Log optimization remarks");
        console.log("  --log-timings               Log compilation timings");
        console.log("  --generate-reports          Generate compilation reports");
        console.log("  --report-output <path>      Set report output path");
        console.log("  --enable-profiling          Enable compiler profiling");
        console.log("  --measure-memory            Measure memory usage");
        console.log("  --warning-level <n>         Warning level (0-3)");
        console.log("  --warnings-as-errors        Treat warnings as errors");
        console.log("  --suppress-warning <id>     Suppress specific warning");
        console.log("");
        console.log("Plugin System:");
        console.log("  --plugin <plugin>           Load compiler plugin");
        console.log("  --plugin-path <path>        Add plugin search path");
        console.log("  --plugin-option <key>=<val> Set plugin option");
        console.log("");
        console.log("Resource Limits:");
        console.log("  --max-compile-time <sec>    Maximum compilation time");
        console.log("  --max-memory <size>         Maximum memory usage");
        console.log("");
        console.log("Environment:");
        console.log("  --working-dir <dir>         Set working directory");
        console.log("  --env <key>=<value>         Set environment variable");
        console.log("  --cpu-features <features>   Set CPU features (native, or specific)");
        console.log("");
        console.log("Examples:");
        console.log("  omniscript --execute hello.os");
        console.log("  omniscript --make --target-arch arm64 --target-os linux app.os");
        console.log("  omniscript --emit-staticlib --optimization-level 3 lib.os");
        console.log("  omniscript --dry --emit-assembly --log-asm code.os");
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

    static size_t parseSizeString(const std::string& sizeStr) {
        if (sizeStr.empty()) {
            throw std::invalid_argument("Empty size string");
        }
        
        // Create a copy and convert to uppercase for case-insensitive parsing
        std::string str = sizeStr;
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        
        // Find where the numeric part ends and suffix begins
        size_t suffixPos = str.find_first_not_of("0123456789.");
        
        // Extract numeric part
        std::string numericPart = str.substr(0, suffixPos);
        if (numericPart.empty()) {
            throw std::invalid_argument("Invalid size string: no numeric part");
        }
        
        // Parse the numeric value (supports floating point)
        double value;
        try {
            value = std::stod(numericPart);
        } catch (const std::exception&) {
            throw std::invalid_argument("Invalid numeric value in size string: " + numericPart);
        }
        
        if (value < 0) {
            throw std::invalid_argument("Size cannot be negative");
        }
        
        // Helper lambda to try parsing with a specific multiplier base
        auto tryParse = [&](bool useBinary) -> std::pair<bool, size_t> {
            size_t multiplier = 1;
            
            // Parse suffix if present
            if (suffixPos != std::string::npos) {
                std::string suffix = str.substr(suffixPos);
                
                // Remove trailing whitespace
                suffix.erase(suffix.find_last_not_of(" \t\n\r\f\v") + 1);
                
                if (suffix == "B" || suffix.empty()) {
                    multiplier = 1;
                } else if (suffix == "KB" || suffix == "K") {
                    multiplier = useBinary ? 1024ULL : 1000ULL;
                } else if (suffix == "MB" || suffix == "M") {
                    multiplier = useBinary ? (1024ULL * 1024ULL) : (1000ULL * 1000ULL);
                } else if (suffix == "GB" || suffix == "G") {
                    multiplier = useBinary ? (1024ULL * 1024ULL * 1024ULL) : (1000ULL * 1000ULL * 1000ULL);
                } else if (suffix == "TB" || suffix == "T") {
                    multiplier = useBinary ? (1024ULL * 1024ULL * 1024ULL * 1024ULL) : (1000ULL * 1000ULL * 1000ULL * 1000ULL);
                } else if (suffix == "PB" || suffix == "P") {
                    multiplier = useBinary ? (1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL) : (1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL);
                } else {
                    return {false, 0}; // Unknown suffix
                }
            }
            
            // Calculate final size and check for overflow
            double result = value * multiplier;
            if (result > static_cast<double>(SIZE_MAX)) {
                return {false, 0}; // Overflow
            }
            
            return {true, static_cast<size_t>(result)};
        };
        
        // First try binary prefixes (1024-based)
        auto binaryResult = tryParse(true);
        if (binaryResult.first) {
            return binaryResult.second;
        }
        
        // Fall back to SI prefixes (1000-based)
        auto siResult = tryParse(false);
        if (siResult.first) {
            return siResult.second;
        }
        
        // If both failed, throw an error
        std::string suffix = (suffixPos != std::string::npos) ? str.substr(suffixPos) : "";
        if (!suffix.empty()) {
            throw std::invalid_argument("Unknown size suffix or value too large: " + suffix);
        } else {
            throw std::invalid_argument("Size value too large");
        }
    }

    // Helper function to check if a size string uses SI prefixes
    static bool isLikelySIPrefix(const std::string& sizeStr) {
        std::string str = sizeStr;
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        
        // Common indicators that SI prefixes might be intended
        return str.find("1000") != std::string::npos ||
            str.find("DECIMAL") != std::string::npos ||
            str.find("SI") != std::string::npos;
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
            
            // Additional Output Formats
            } else if (arg == "--emit-object") {
                config.mode = CompileMode::AOT;
                config.aot.outputFormat = OutputFormat::ObjectFile;
            } else if (arg == "--emit-bitcode") {
                config.mode = CompileMode::AOT;
                config.aot.outputFormat = OutputFormat::Bitcode;
            } else if (arg == "--emit-machine-code") {
                config.mode = CompileMode::AOT;
                config.aot.outputFormat = OutputFormat::MachineCode;
            } else if (arg == "--emit-wasm") {
                config.mode = CompileMode::AOT;
                config.aot.outputFormat = OutputFormat::WebAssembly;
            } else if (arg == "--emit-ptx") {
                config.mode = CompileMode::AOT;
                config.aot.outputFormat = OutputFormat::PTX;
            } else if (arg == "--emit-spirv") {
                config.mode = CompileMode::AOT;
                config.aot.outputFormat = OutputFormat::SPIR_V;
            
            // JIT Configuration
            } else if (arg == "--jit-engine") {
                if (i + 1 < argc) {
                    std::string engine = argv[++i];
                    if (engine == "orc") config.jit.engine = JITEngine::LLVM_ORC;
                    else if (engine == "mcjit") config.jit.engine = JITEngine::LLVM_MCJIT;
                    else if (engine == "custom") config.jit.engine = JITEngine::Custom;
                    else console.error("Error: Invalid JIT engine. Use: orc, mcjit, custom");
                } else {
                    console.error("Error: Missing JIT engine after '--jit-engine'.");
                }
            } else if (arg == "--jit-lazy") {
                config.jit.lazyCompilation = true;
            } else if (arg == "--jit-speculation") {
                config.jit.enableSpeculation = true;
            } else if (arg == "--jit-threshold") {
                if (i + 1 < argc) {
                    config.jit.compilationThreshold = std::stoul(argv[++i]);
                } else {
                    console.error("Error: Missing threshold after '--jit-threshold'.");
                }
            } else if (arg == "--jit-tiered") {
                config.jit.enableTieredCompilation = true;
            } else if (arg == "--jit-cache-size") {
                if (i + 1 < argc) {
                    config.jit.maxCodeCacheSize = std::stoul(argv[++i]) * 1024 * 1024; // Convert MB to bytes
                } else {
                    console.error("Error: Missing cache size after '--jit-cache-size'.");
                }
            
            // Runtime Configuration
            } else if (arg == "--heap-size") {
                if (i + 1 < argc) {
                    config.runtime.heapSize = parseSizeString(argv[++i]);
                } else {
                    console.error("Error: Missing heap size after '--heap-size'.");
                }
            } else if (arg == "--stack-size") {
                if (i + 1 < argc) {
                    config.runtime.stackSize = parseSizeString(argv[++i]);
                } else {
                    console.error("Error: Missing stack size after '--stack-size'.");
                }
            } else if (arg == "--gc-threads") {
                if (i + 1 < argc) {
                    config.runtime.gcThreads = std::stoi(argv[++i]);
                } else {
                    console.error("Error: Missing thread count after '--gc-threads'.");
                }
            } else if (arg == "--enable-parallel-gc") {
                config.runtime.enableParallelGC = true;
            } else if (arg == "--enable-concurrent-gc") {
                config.runtime.enableConcurrentGC = true;
            
            // Security Options
            } else if (arg == "--enable-stack-protection") {
                config.security.enableStackProtection = true;
            } else if (arg == "--enable-cfi") {
                config.security.enableControlFlowIntegrity = true;
            } else if (arg == "--enable-asan") {
                config.security.enableAddressSanitizer = true;
            } else if (arg == "--enable-msan") {
                config.security.enableMemorySanitizer = true;
            } else if (arg == "--enable-tsan") {
                config.security.enableThreadSanitizer = true;
            } else if (arg == "--enable-ubsan") {
                config.security.enableUndefinedBehaviorSanitizer = true;
            } else if (arg == "--enable-pic") {
                config.security.enablePositionIndependentCode = true;
            } else if (arg == "--enable-dep") {
                config.security.enableDataExecutionPrevention = true;
            } else if (arg == "--enable-aslr") {
                config.security.enableAddressSpaceLayoutRandomization = true;
            
            // Optimization Options
            } else if (arg == "--enable-vectorization") {
                config.optimization.enableVectorization = true;
            } else if (arg == "--enable-loop-unrolling") {
                config.optimization.enableLoopUnrolling = true;
            } else if (arg == "--enable-inlining") {
                config.optimization.enableFunctionInlining = true;
            } else if (arg == "--enable-tail-calls") {
                config.optimization.enableTailCallOptimization = true;
            } else if (arg == "--enable-dead-code-elim") {
                config.optimization.enableDeadCodeElimination = true;
            } else if (arg == "--enable-const-folding") {
                config.optimization.enableConstantFolding = true;
            } else if (arg == "--enable-cse") {
                config.optimization.enableCommonSubexpressionElimination = true;
            } else if (arg == "--enable-licm") {
                config.optimization.enableLoopInvariantCodeMotion = true;
            } else if (arg == "--fast-math") {
                config.optimization.fastMath = true;
            } else if (arg == "--pgo-profile") {
                if (i + 1 < argc) {
                    config.optimization.pgo.profileDataPath = argv[++i];
                } else {
                    console.error("Error: Missing profile path after '--pgo-profile'.");
                }
            } else if (arg == "--pgo-instrument") {
                config.optimization.pgo.instrumentForProfiling = true;
            
            // Linking Options
            } else if (arg == "--library-path") {
                if (i + 1 < argc) {
                    config.aot.libraryPaths.push_back(argv[++i]);
                } else {
                    console.error("Error: Missing path after '--library-path'.");
                }
            } else if (arg == "--library") {
                if (i + 1 < argc) {
                    config.aot.libraries.push_back(argv[++i]);
                } else {
                    console.error("Error: Missing library name after '--library'.");
                }
            } else if (arg == "--framework-path") {
                if (i + 1 < argc) {
                    config.aot.frameworkPaths.push_back(argv[++i]);
                } else {
                    console.error("Error: Missing path after '--framework-path'.");
                }
            } else if (arg == "--framework") {
                if (i + 1 < argc) {
                    config.aot.frameworks.push_back(argv[++i]);
                } else {
                    console.error("Error: Missing framework name after '--framework'.");
                }
            } else if (arg == "--linker-script") {
                if (i + 1 < argc) {
                    config.aot.linkerScript = argv[++i];
                } else {
                    console.error("Error: Missing script path after '--linker-script'.");
                }
            } else if (arg == "--linker-flag") {
                if (i + 1 < argc) {
                    config.aot.linkerFlags.push_back(argv[++i]);
                } else {
                    console.error("Error: Missing flag after '--linker-flag'.");
                }
            } else if (arg == "--static-linking") {
                config.aot.staticLinking = true;
            } else if (arg == "--strip-symbols") {
                config.aot.stripSymbols = true;
            } else if (arg == "--debug-info") {
                config.aot.generateDebugInfo = true;
            } else if (arg == "--debug-format") {
                if (i + 1 < argc) {
                    config.aot.debugInfoFormat = argv[++i];
                } else {
                    console.error("Error: Missing format after '--debug-format'.");
                }
            } else if (arg == "--thin-lto") {
                config.aot.lto.thinLTO = true;
            } else if (arg == "--lto-jobs") {
                if (i + 1 < argc) {
                    config.aot.lto.parallelJobs = std::stoi(argv[++i]);
                } else {
                    console.error("Error: Missing job count after '--lto-jobs'.");
                }
            
            // Build Configuration
            } else if (arg == "--temp-dir") {
                if (i + 1 < argc) {
                    config.tempDirectory = argv[++i];
                } else {
                    console.error("Error: Missing directory after '--temp-dir'.");
                }
            } else if (arg == "--parallel-jobs") {
                if (i + 1 < argc) {
                    config.parallelJobs = std::stoi(argv[++i]);
                } else {
                    console.error("Error: Missing job count after '--parallel-jobs'.");
                }
            } else if (arg == "--enable-caching") {
                config.enableCaching = true;
            } else if (arg == "--cache-dir") {
                if (i + 1 < argc) {
                    config.cacheDirectory = argv[++i];
                } else {
                    console.error("Error: Missing directory after '--cache-dir'.");
                }
            } else if (arg == "--include-path") {
                if (i + 1 < argc) {
                    config.includePaths.push_back(argv[++i]);
                } else {
                    console.error("Error: Missing path after '--include-path'.");
                }
            } else if (arg == "--source-path") {
                if (i + 1 < argc) {
                    config.sourcePaths.push_back(argv[++i]);
                } else {
                    console.error("Error: Missing path after '--source-path'.");
                }
            } else if (arg == "--import-path") {
                if (i + 1 < argc) {
                    config.importPaths.push_back(argv[++i]);
                } else {
                    console.error("Error: Missing path after '--import-path'.");
                }
            } else if (arg == "--define") {
                if (i + 1 < argc) {
                    std::string define = argv[++i];
                    size_t eq = define.find('=');
                    if (eq != std::string::npos) {
                        config.defines[define.substr(0, eq)] = define.substr(eq + 1);
                    } else {
                        config.defines[define] = "1";
                    }
                } else {
                    console.error("Error: Missing definition after '--define'.");
                }
            } else if (arg == "--undefine") {
                if (i + 1 < argc) {
                    config.undefines.push_back(argv[++i]);
                } else {
                    console.error("Error: Missing macro name after '--undefine'.");
                }
            
            // Language Features
            } else if (arg == "--std") {
                if (i + 1 < argc) {
                    config.languageStandard = argv[++i];
                } else {
                    console.error("Error: Missing standard after '--std'.");
                }
            } else if (arg == "--enable-experimental") {
                config.enableExperimentalFeatures = true;
            } else if (arg == "--enable-feature") {
                if (i + 1 < argc) {
                    config.enabledFeatures.push_back(argv[++i]);
                } else {
                    console.error("Error: Missing feature name after '--enable-feature'.");
                }
            } else if (arg == "--disable-feature") {
                if (i + 1 < argc) {
                    config.disabledFeatures.push_back(argv[++i]);
                } else {
                    console.error("Error: Missing feature name after '--disable-feature'.");
                }
            
            // Diagnostics
            } else if (arg == "--log-optimization") {
                config.diagnostics.logOptimizationRemarks = true;
            } else if (arg == "--log-timings") {
                config.diagnostics.logTimings = true;
            } else if (arg == "--generate-reports") {
                config.diagnostics.generateReports = true;
            } else if (arg == "--report-output") {
                if (i + 1 < argc) {
                    config.diagnostics.reportOutputPath = argv[++i];
                } else {
                    console.error("Error: Missing path after '--report-output'.");
                }
            } else if (arg == "--enable-profiling") {
                config.diagnostics.enableProfiling = true;
            } else if (arg == "--measure-memory") {
                config.diagnostics.measureMemoryUsage = true;
            } else if (arg == "--warning-level") {
                if (i + 1 < argc) {
                    config.diagnostics.warningLevel = std::stoi(argv[++i]);
                } else {
                    console.error("Error: Missing level after '--warning-level'.");
                }
            } else if (arg == "--warnings-as-errors") {
                config.diagnostics.warningsAsErrors = true;
            } else if (arg == "--suppress-warning") {
                if (i + 1 < argc) {
                    config.diagnostics.suppressedWarnings.push_back(argv[++i]);
                } else {
                    console.error("Error: Missing warning ID after '--suppress-warning'.");
                }
            
            // Plugin System
            } else if (arg == "--plugin") {
                if (i + 1 < argc) {
                    config.plugins.push_back(argv[++i]);
                } else {
                    console.error("Error: Missing plugin name after '--plugin'.");
                }
            } else if (arg == "--plugin-path") {
                if (i + 1 < argc) {
                    config.pluginPaths.push_back(argv[++i]);
                } else {
                    console.error("Error: Missing path after '--plugin-path'.");
                }
            } else if (arg == "--plugin-option") {
                if (i + 1 < argc) {
                    std::string option = argv[++i];
                    size_t eq = option.find('=');
                    if (eq != std::string::npos) {
                        config.pluginOptions[option.substr(0, eq)] = option.substr(eq + 1);
                    } else {
                        console.error("Error: Plugin option must be in format key=value");
                    }
                } else {
                    console.error("Error: Missing option after '--plugin-option'.");
                }
            
            // Resource Limits
            } else if (arg == "--max-compile-time") {
                if (i + 1 < argc) {
                    config.maxCompilationTime = std::stoul(argv[++i]);
                } else {
                    console.error("Error: Missing time after '--max-compile-time'.");
                }
            } else if (arg == "--max-memory") {
                if (i + 1 < argc) {
                    config.maxMemoryUsage = parseSizeString(argv[++i]);
                } else {
                    console.error("Error: Missing size after '--max-memory'.");
                }
            
            // Environment
            } else if (arg == "--working-dir") {
                if (i + 1 < argc) {
                    config.workingDirectory = argv[++i];
                } else {
                    console.error("Error: Missing directory after '--working-dir'.");
                }
            } else if (arg == "--env") {
                if (i + 1 < argc) {
                    std::string env = argv[++i];
                    size_t eq = env.find('=');
                    if (eq != std::string::npos) {
                        config.environmentVariables[env.substr(0, eq)] = env.substr(eq + 1);
                    } else {
                        console.error("Error: Environment variable must be in format key=value");
                    }
                } else {
                    console.error("Error: Missing variable after '--env'.");
                }
            } else if (arg == "--cpu-features") {
                if (i + 1 < argc) {
                    config.cpuFeatures = argv[++i];
                } else {
                    console.error("Error: Missing features after '--cpu-features'.");
                }
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