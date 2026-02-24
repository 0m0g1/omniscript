#include <omniscript/Engine.h>
#include <omniscript/backends/Backend.h>
#include <omniscript/backends/llvm/LLVMJITBackend.h>
#include <omniscript/backends/llvm/LLVMAOTBackend.h>

#include <omniscript/lexer/Lexer.h>
#include <omniscript/parser/Parser.h>
#include <omniscript/ast/AstPrint.h>
#include <omniscript/extern/ExternResolver.h>
#include <omniscript/semantics/SymbolTable.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace Omniscript {

Engine::Engine(int argc, char** argv)
    : m_argc(argc), m_argv(argv) {}

std::string Engine::readSourceFile(const std::string& file_path) const {
    std::ifstream in(file_path, std::ios::in | std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open file: " + file_path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::expected<Config, std::string>
Engine::parseArguments(int argc, char** argv) noexcept {
    if (argc < 2 || !argv || !argv[1]) {
        return std::unexpected("Usage: omniscript <source-file> [--execute|--make|--dry] [-o <out>] [-I <dir> ...] [--emit-ir|--emit-object|--emit-assembly]");
    }

    Config cfg;
    cfg.filePath = argv[1];
    cfg.mainSourceFile = argv[1];
    cfg.sourcePaths = { argv[1] };

    // defaults
    cfg.mode = CompileMode::JIT;
    cfg.outputPath = "a.out";

    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];

        if (a == "--execute") cfg.mode = CompileMode::JIT;
        else if (a == "--make") cfg.mode = CompileMode::AOT;
        else if (a == "--dry") cfg.mode = CompileMode::DryCompile;

        else if ((a == "-o" || a == "--output") && i + 1 < argc) cfg.outputPath = argv[++i];

        else if ((a == "-I" || a == "--include") && i + 1 < argc) cfg.includePaths.push_back(argv[++i]);
        else if (a.size() > 2 && a[0] == '-' && a[1] == 'I') cfg.includePaths.push_back(a.substr(2));

        else if (a == "--emit-ir")      { cfg.mode = CompileMode::AOT; cfg.aot.outputFormat = OutputFormat::LLVM_IR; }
        else if (a == "--emit-object")  { cfg.mode = CompileMode::AOT; cfg.aot.outputFormat = OutputFormat::ObjectFile; }
        else if (a == "--emit-assembly"){ cfg.mode = CompileMode::AOT; cfg.aot.outputFormat = OutputFormat::Assembly; }

        else if (a == "--debug") cfg.diagnostics.debugMode = true;
        else if (a == "--verbose") cfg.diagnostics.verbose = true;

        else {
            // ignore unknown flags for now or fail fast:
            // return std::unexpected("Unknown argument: " + a);
        }
    }

    cfg.autoConfigureForTarget();

    std::string err;
    if (!cfg.validate(err)) return std::unexpected("Configuration validation failed: " + err);

    return cfg;
}

std::unique_ptr<Backend> Engine::makeBackend(const Config& config) {
#if OMNI_HAS_LLVM
    if (config.isAOTMode()) return std::make_unique<LLVMAOTBackend>();
    return std::make_unique<LLVMJITBackend>();
#else
    (void)config;
    throw std::runtime_error("LLVM backend not available (OMNI_HAS_LLVM=0)");
#endif
}

int Engine::run() {
    try {
        auto cfgOrErr = parseArguments(m_argc, m_argv);
        if (!cfgOrErr) {
            std::cerr << cfgOrErr.error() << "\n";
            return 2;
        }
        Config config = std::move(*cfgOrErr);

        const std::string sourcePath = config.mainSourceFile.empty() ? config.filePath : config.mainSourceFile;
        const std::string source     = readSourceFile(sourcePath);

        // ---- 1) Parse ----
        Lexer  lexer(source, sourcePath.c_str());
        Parser parser(lexer);
        auto   program = parser.parse();

        // ---- 2) FFI expansion ----
        {
            namespace fs = std::filesystem;
            extern_support::ResolverConfig cfg;

            cfg.includeDirs.push_back(
                fs::path(sourcePath).has_parent_path()
                    ? fs::path(sourcePath).parent_path().string()
                    : std::string(".")
            );

            // integrate EngineConfigs includePaths
            for (const auto& inc : config.includePaths) cfg.includeDirs.push_back(inc);

            cfg.debugPrint = config.diagnostics.debugMode;
            cfg.debugOut   = &std::cout;

            cfg.emitOpts.sourceComments   = true;
            cfg.emitOpts.skipPrivateNames = true;
            cfg.emitOpts.structsAsOpaque  = false;

            extern_support::ExternResolver resolver(
                std::move(cfg),
                fs::path(sourcePath).has_parent_path()
                    ? fs::path(sourcePath).parent_path().string()
                    : std::string(".")
            );

            resolver.expand(*program);
        }

        // ---- 3) Optional AST print ----
        if (config.diagnostics.debugMode) {
            AstPrinter printer(std::cout, PrintMode::Recursive);
            printer.print(*program);
        }

        // ---- 4) Semantic checks ----
        {
            SymbolTable globalScope(nullptr);
            EvalContext ctx;

            const bool ok = program->evaluate(globalScope, ctx);
            if (!ok || ctx.diags.hasErrors()) {
                ctx.diags.print(std::cerr);
                return 1;
            }
        }

        // ---- 5) Backend dispatch ----
        auto backend = makeBackend(config);
        backend->initialize(config);
        backend->execute(*program, config);

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

} // namespace Omniscript