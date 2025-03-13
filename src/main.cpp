#include <omniscript/utils.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/parser.h>
#include <omniscript/omniscript_pch.h>

struct Config {
    bool debugMode = false;
    bool executeStatements = false;
    bool useCompiler = false;
    std::string filePath;
};

class Interpreter {
public:
    void interpret(const std::string& source, const Config &config);
};

void Interpreter::interpret(const std::string& source, const Config &config) {
    Lexer lexer(source);
    Parser parser(lexer);
    parser.setScopeName();
    parser.setDebugMode(config.debugMode);
    parser.setExecution(config.executeStatements);
    parser.Parse();
    parser.Interprete();
}

class Compiler {
public:
    void compile(const std::string& source, const Config &config) {
        Lexer lexer(source);
        Parser parser(lexer);
        parser.setScopeName();
        parser.setDebugMode(config.debugMode);
        
        console.debug("Compiling source code...");

        parser.Parse();
        parser.Compile();

        // Add further compilation logic here, e.g., LLVM IR generation.
        console.log("Compilation completed.");
    }
};

class Engine {
public:
    static constexpr const char* VERSION = "1.0.0";

    static void printUsage() {
        console.log("Usage: omniscript [options] <file>");
        console.log("Options:");
        console.log("  --debug       Enable debug mode");
        console.log("  --execute     Execute statements");
        console.log("  --make        Compile the source code");
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

        if (config.useCompiler) {
            Compiler compiler;
            compiler.compile(sourceCode, config);
        } else {
            Interpreter interpreter;
            interpreter.interpret(sourceCode, config);
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
