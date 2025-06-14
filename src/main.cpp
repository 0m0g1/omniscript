#include <omniscript/utils.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/parser.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/JITCompiler.h>
#include <omniscript/engine/Backends/JITBackend.h>
#include <omniscript/engine/Backends/llvm/LLVMJITBackend.h>
#include <omniscript/engine/Backends/llvm/LLVMAOTBackend.h>
#include <omniscript/engine/EngineConfigs.h>

class Compiler {
public:
    void compile(const std::vector<std::shared_ptr<Statement>>& statements, const Config &config) {
        DEBUG_LOG("Compiling source code with AOT backend...");
        auto backend = std::make_shared<LLVMAOTBackend>();
        backend->initialize();
        backend->execute(statements, config);
        console.log("AOT Compilation completed.");
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
        console.log("  --emit-staticlib         Emit a static library (future)");
        console.log("  --emit-sharedlib         Emit a shared library (future)");
        console.log("  --execute                Execute statements using JIT");
        console.log("  --keep-obj               Keep intermediate object files");
        console.log("  --log-asm                Log the final generated assembly code");
        console.log("  --log-final-code         Log the final generated IR code");
        console.log("  --make                   Compile the source code (AOT)");
        console.log("  --output, -o <file>      Set output file path for AOT");
        console.log("  --optimization-level, -O <n> Optimization level: -1 (none) to 2 (max)");
        console.log("  --version                Display version information");
        console.log("  --help                   Display this help message");
        console.log("  --verbose                Show full parse tree and token stream");
    }

    static Config parseArguments(int argc, char* argv[]) {
        Config config;
        bool fileSpecified = false;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--debug" || arg == "-d") {
                config.debugMode = true;
                console.enableDebug();
            } else if (arg == "--verbose") {
                config.verboseMode = true;
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
            } else if (arg == "--entry") {
                if (i + 1 < argc) {
                    config.entry = argv[++i];
                } else {
                    console.error("Error: Missing function name after '--entry'.");
                }
            } else if (arg == "--log-asm") {
                config.logAsm = true;
            } else if (arg == "--log-final-code") {
                config.logFinalCode = true;
            } else if (arg == "--optimization-level" || arg == "-O") {
                if (i + 1 < argc) {
                    config.optimizationLevel = std::stoi(argv[++i]);
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
                config.showMetadata = true;
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

        return config;
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
        DEBUG_LOG("Config State:");
        DEBUG_LOG("File: " + config.filePath);
        DEBUG_LOG("Mode: " + std::to_string(static_cast<int>(config.mode)));
        DEBUG_LOG("Entry: " + config.entry);
        DEBUG_LOG("Output Path: " + config.outputPath);
        DEBUG_LOG("Optimization Level: " + std::to_string(config.optimizationLevel));
        DEBUG_LOG("Debug Mode: " + std::string(config.debugMode ? "true" : "false"));
        DEBUG_LOG("Log IR: " + std::string(config.logFinalCode ? "true" : "false"));
        DEBUG_LOG("Log ASM: " + std::string(config.logAsm ? "true" : "false"));
        DEBUG_LOG("Keep Obj: " + std::string(config.keepIntermediateFiles ? "true" : "false"));
    }

    static void run(const Config& config) {
        std::string sourceCode = readSourceCode(config);

        Lexer lexer(sourceCode, config.filePath);
        Parser parser(lexer);
        parser.setDebugMode(config.debugMode);
        std::vector<std::shared_ptr<Statement>> statements = parser.Parse();

        if (config.verboseMode) {
            parser.printParseTree();
            parser.printTokenStream();
        }

        if (config.debugMode) {
            printConfigDebugInfo(config);
        }

        if (config.mode == CompileMode::AOT || config.mode == CompileMode::DryCompile) {
            Compiler compiler;
            compiler.compile(statements, config);
            if (config.mode == CompileMode::AOT) {
                console.log("Compilation done. Executable or object emitted to: " + config.outputPath);
            } else if (config.mode == CompileMode::DryCompile) {
                console.log("Dry compilation complete. Output written to: " + config.outputPath);
            }
        } else {
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
