#include <omniscript/Engine.h>
#include <omniscript/lexer/Lexer.h>
#include <omniscript/parser/Parser.h>
#include <omniscript/ast/Ast.h>
#include <omniscript/ast/AstPrint.h>
#include <omniscript/ast/AstStatement.h>
#include <omniscript/extern/CHeaderParser.h>
#include <omniscript/extern/OsEmitter.h>     // <-- ADD THIS

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

        Lexer  lexer(source, sourcePath.c_str());
        Parser parser(lexer);
        auto   program = parser.parse();

        std::cout << "Parsed " << program->statements.size() << " statements\n";

        // ---- PRINT AST ----
        {
            AstPrinter printer(std::cout, PrintMode::Recursive);
            printer.print(*program);
        }

        // ================================================================
        // FFI: parse every extern header and emit the .os equivalent
        // ================================================================
        namespace fs = std::filesystem;
        const fs::path baseDir = fs::path(sourcePath).has_parent_path()
                                 ? fs::path(sourcePath).parent_path()
                                 : fs::path(".");

        // One emitter, shared across all extern blocks in the file
        extern_support::EmitOptions emitOpts;
        emitOpts.sourceComments   = true;   // // method comments
        emitOpts.skipPrivateNames = true;   // skip _foo names
        emitOpts.structsAsOpaque  = false;  // show fields
        extern_support::OsEmitter emitter(emitOpts);

        for (const auto& st : program->statements) {
            if (!st) continue;
            auto* ex = dynamic_cast<const ExternStmt*>(st.get());
            if (!ex || ex->headerPaths.empty()) continue;

            for (const auto& hp : ex->headerPaths) {
                // Resolve the header path relative to the .os source file
                fs::path headerPath = fs::path(hp);
                if (headerPath.is_relative())
                    headerPath = (baseDir / headerPath).lexically_normal();
                const std::string resolved = headerPath.string();

                std::cout << "\n// " << std::string(72, '=') << '\n';
                std::cout << "// Header : " << hp
                          << "  (resolved: " << resolved << ")\n";
                std::cout << "// " << std::string(72, '=') << '\n';

                try {
                    const std::string headerSrc = readSourceFile(resolved);

                    extern_support::CHeaderParser p(headerSrc, resolved);
                    const extern_support::CHeaderResult result = p.parse();

                    // Print the .os extern block
                    emitter.emit(std::cout, result, ex->libraryPaths);

                } catch (const std::exception& e) {
                    std::cout << "// ERROR parsing '" << hp << "': " << e.what() << "\n";
                }
            }
        }
        // ================================================================

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

} // namespace Omniscript