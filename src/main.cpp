#include <omniscript/utils.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/parser.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/JITCompiler.h>
#include <omniscript/engine/Backends/JITBackend.h>
#include <omniscript/engine/Backends/llvm/LLVMJITBackend.h>
#include <omniscript/engine/EngineConfigs.h>

class Compiler {
private:

public:
    Compiler() {}
    void compile(const std::vector<std::shared_ptr<Statement>>& statements, const Config &config);
};

void Compiler::compile(const std::vector<std::shared_ptr<Statement>>& statements, const Config &config) {
    DEBUG_LOG("Compiling source code...");
    for (const auto& statement : statements) {
        // llvm::Value* ir = statement->codegen(irGen);
        // Further compilation logic (e.g., emitting LLVM IR)
    }
    console.log("Compilation completed.");
}

class Engine {
public:
    static constexpr const char* VERSION = "1.0.0";

    static void printUsage() {
        console.log("Usage: omniscript [options] <file>");
        console.log("Options:");
        console.log("  --debug                  Enable debug mode");
        console.log("  --entry                  The function to call when starting the program");
        console.log("  --emit-staticlib         Execute statements (JIT compilation)");
        console.log("  --emit-sharedlib         Execute statements (JIT compilation)");
        console.log("  --execute                Execute statements (JIT compilation)");
        console.log("  --log-asm                Log The final generated asembly code");
        console.log("  --log-final-code         Log The final generated code");
        console.log("  --make                   Compile the source code (AOT compilation)");
        console.log("  --optimization-level     Optimize the code -1 no optimization 2 max optimization");
        console.log("  --version                Display version information");
        console.log("  --help                   Display this help message");
    }

    static Config parseArguments(int argc, char* argv[]) {
        Config config;
        bool fileSpecified = false;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--debug") {
                config.debugMode = true;
                console.enableDebug();
            } else if (arg == "--execute") {
                config.executeStatements = true;
            } else if (arg == "--make") {
                config.useCompiler = true;
            } else if (arg == "--version") {
                console.log(std::string("Omniscript version ") + VERSION);
                std::exit(0);
            } else if (arg == "--help") {
                printUsage();
                std::exit(0);
            } else if (arg == "--entry") {
                if (i + 1 < argc) {
                    config.entry = argv[i + 1];
                    ++i;
                } else {
                    console.error("Error: Missing function name after '--entry'.");
                }
            } else if (arg == "--log-asm") {
                config.logAsm = true;
            } else if (arg == "--log-final-code") {
                config.logFinalCode = true;
            } else if (arg == "--optimization-level") {
                if (i + 1 < argc) {
                    config.optimizationLevel = std::stoi(argv[i + 1]);
                    ++i;
                } else {
                    console.error("Error: Missing int for optimization level after '--optimization-level'.");
                }
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
            console.error("Error: File path is required.");
        }

        if (config.executeStatements && config.useCompiler) {
            console.error("Error: Conflicting options: cannot use both '--execute' and '--make' simultaneously.");
        }

        if (!config.executeStatements && !config.useCompiler) {
            config.executeStatements = true;
        }

        return config;
    }

    static std::string readSourceCode(const Config& config) {
        std::ifstream file(config.filePath);
        if (!file) {
            console.error("Error opening file: " + config.filePath);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string sourceCode = buffer.str();

        // DEBUG_LOG("Source code loaded:\n" + sourceCode);
        return sourceCode;
    }

    static void run(const Config& config) {
        std::string sourceCode = readSourceCode(config);
        
        Lexer lexer(sourceCode, config.filePath);
        Parser parser(lexer);
        parser.setDebugMode(config.debugMode);
        std::vector<std::shared_ptr<Statement>> statements = parser.Parse();
        
        if (config.useCompiler) {
            // Compiler compiler();
            // compiler.compile(statements, config);
        } else {
            auto backend = std::make_shared<LLVMJITBackend>();
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
