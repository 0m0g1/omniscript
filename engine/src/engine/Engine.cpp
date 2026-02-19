#include <omniscript/Engine.h>
#include <omniscript/lexer/Lexer.h>
#include <omniscript/parser/Parser.h>
#include <omniscript/ast/Ast.h>
#include <omniscript/ast/AstPrint.h>   // <-- add this

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Omniscript {

Engine::Engine(int argc, char** argv)
  : m_argc(argc), m_argv(argv) {}

std::string Engine::readSourceFile(const std::string& file_path) const {
    std::ifstream in(file_path);
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

        const std::string source = readSourceFile(m_argv[1]);

        Lexer lexer(source, m_argv[1]);
        Parser parser(lexer);
        auto program = parser.parse();

        std::cout << "Parsed " << program->statements.size() << " statements\n";

        // ---- PRINT AST HERE ----
        {
            // toggle this how you want
            const bool recursive = true;

            AstPrinter printer(
                std::cout,
                recursive ? PrintMode::Recursive : PrintMode::NonRecursive
            );
            printer.print(*program);
        }
        // ------------------------

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

} // namespace Omniscript
