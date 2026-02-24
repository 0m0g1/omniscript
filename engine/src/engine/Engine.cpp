#include <omniscript/Engine.h>
#include <omniscript/lexer/Lexer.h>
#include <omniscript/parser/Parser.h>
#include <omniscript/ast/Ast.h>
#include <omniscript/ast/AstPrint.h>
#include <omniscript/extern/ExternResolver.h>  // <-- FFI expansion
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

int Engine::run() {
    try {
        if (m_argc < 2 || !m_argv || !m_argv[1]) {
            std::cerr << "Usage: " << (m_argv && m_argv[0] ? m_argv[0] : "omniscript")
                      << " <source-file>\n";
            return 2;
        }

        const std::string sourcePath = m_argv[1];
        const std::string source     = readSourceFile(sourcePath);

        // ---- 1. Parse the .os source file ----
        Lexer  lexer(source, sourcePath.c_str());
        Parser parser(lexer);
        auto program = parser.parse();

        std::cout << "Parsed " << program->statements.size()
                  << " statements (before FFI expansion)\n";

        // ---- 2. Expand extern headers -> inject AST nodes ----
        {
            namespace fs = std::filesystem;

            extern_support::ResolverConfig cfg;

            // Always search in the .os file's own directory first
            cfg.includeDirs.push_back(
                fs::path(sourcePath).has_parent_path()
                    ? fs::path(sourcePath).parent_path().string()
                    : std::string(".")
            );

            // Accept extra -I / --include dirs from the command line
            for (int i = 2; i < m_argc; ++i) {
                std::string arg = m_argv[i];
                if ((arg == "--include" || arg == "-I") && i + 1 < m_argc)
                    cfg.includeDirs.push_back(m_argv[++i]);
                else if (arg.size() > 2 && arg[0] == '-' && arg[1] == 'I')
                    cfg.includeDirs.push_back(arg.substr(2));
            }

            // During development: print the generated .os text before parsing it
            cfg.debugPrint = true;
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

            // Mutates program.statements in-place:
            // for every ExternStmt with header paths, appends the
            // generated FunctionDeclStmt / VarDeclStmt / etc. right after it.
            resolver.expand(*program);
        }

        std::cout << "\nAfter FFI expansion: "
                  << program->statements.size() << " statements\n\n";

        // ---- 3. Print the full expanded AST ----
        {
            AstPrinter printer(std::cout, PrintMode::Recursive);
            printer.print(*program);
        }

        // ---- 4. Evaluate / semantic checks ----
        {
            SymbolTable globalScope(nullptr);
            EvalContext ctx;

            // (Optional) define builtins here:
            // globalScope.define({SymbolKind::Function, "print", Type::Function({Type::String()}, Type::Void()), false, {}}, ctx.diags);

            const bool ok = program->evaluate(globalScope, ctx);

            if (!ok || ctx.diags.hasErrors()) {
                ctx.diags.print(std::cerr);
                return 1;
            }
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

} // namespace Omniscript