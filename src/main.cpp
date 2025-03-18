#include <omniscript/utils.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/parser.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/runtime/Statement.h>
#include <omniscript/engine/IRGenerator.h>
#include <omniscript/engine/JITCompiler.h>
#include <omniscript/engine/EngineConfigs.h>

class Compiler {
private:
    IRGenerator& irGen;
public:
    Compiler(IRGenerator& generator) : irGen(generator) {}
    void compile(const std::vector<std::shared_ptr<Statement>>& statements, const Config &config);
};

void Compiler::compile(const std::vector<std::shared_ptr<Statement>>& statements, const Config &config) {
    console.debug("Compiling source code...");
    for (const auto& statement : statements) {
        llvm::Value* ir = static_cast<StatementCRTP<decltype(*statement.get())>*>(statement.get())->generateIR(irGen);
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
        console.log("  --debug       Enable debug mode");
        console.log("  --execute     Execute statements (JIT compilation)");
        console.log("  --make        Compile the source code (AOT compilation)");
        console.log("  --version     Display version information");
        console.log("  --help        Display this help message");
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
            throw std::runtime_error("Error: File path is required.");
        }

        if (config.executeStatements && config.useCompiler) {
            throw std::runtime_error("Error: Conflicting options: cannot use both '--execute' and '--make' simultaneously.");
        }

        if (!config.executeStatements && !config.useCompiler) {
            config.executeStatements = true;
        }

        return config;
    }

    static std::string readSourceCode(const Config& config) {
        std::ifstream file(config.filePath);
        if (!file) {
            throw std::runtime_error("Error opening file: " + config.filePath);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string sourceCode = buffer.str();

        if (config.debugMode) {
            console.debug("Source code loaded:\n" + sourceCode);
        }
        return sourceCode;
    }

    static void run(const Config& config) {
        std::string sourceCode = readSourceCode(config);
        Lexer lexer(sourceCode);
        Parser parser(lexer);
        parser.setScopeName();
        parser.setDebugMode(config.debugMode);
        std::vector<std::shared_ptr<Statement>> statements = parser.Parse();
        
        IRGenerator irGen;
        if (config.useCompiler) {
            Compiler compiler(irGen);
            compiler.compile(statements, config);
        } else {
            JITCompiler jit(irGen);
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
