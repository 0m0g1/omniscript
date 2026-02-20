// engine/src/omniscript/extern/ExternResolver.cpp
//
// Expands ExternStmt header paths into real OmniScript AST nodes.
//
// Pipeline:
//   1. CHeaderParser  — parse the C/C++ header into CHeaderResult
//   2. OsEmitter      — convert CHeaderResult to .os source text
//   3. Lexer + Parser — re-parse the .os text through the existing parser
//   4. Inject         — insert resulting StmtPtrs after the ExternStmt

#include <omniscript/extern/ExternResolver.h>
#include <omniscript/lexer/Lexer.h>
#include <omniscript/parser/Parser.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace Omniscript {
namespace extern_support {

// =============================================================================
// Construction
// =============================================================================

ExternResolver::ExternResolver(ResolverConfig cfg, std::string sourceDir)
    : m_cfg(std::move(cfg))
    , m_sourceDir(std::move(sourceDir))
    , m_emitter(m_cfg.emitOpts)
{}

// =============================================================================
// File helpers
// =============================================================================

std::string ExternResolver::findHeader(const std::string& hp) const {
    // 1. Absolute path
    {
        fs::path p(hp);
        if (p.is_absolute() && fs::exists(p)) return p.string();
    }

    // 2. Relative to the .os source file's directory
    {
        fs::path p = fs::path(m_sourceDir) / hp;
        if (fs::exists(p)) return p.lexically_normal().string();
    }

    // 3. Each extra include directory in order
    for (const auto& dir : m_cfg.includeDirs) {
        fs::path p = fs::path(dir) / hp;
        if (fs::exists(p)) return p.lexically_normal().string();
    }

    return {};
}

std::string ExternResolver::readFile(const std::string& path) const {
    std::ifstream f(path, std::ios::in | std::ios::binary);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// =============================================================================
// Step 2: CHeaderResult -> .os source text
// =============================================================================

std::string ExternResolver::emitToString(const CHeaderResult& result,
                                          const std::vector<std::string>& libPaths) const {
    std::ostringstream ss;
    m_emitter.emit(ss, result, libPaths);
    return ss.str();
}

// =============================================================================
// Step 3: .os source text -> vector<StmtPtr>
// =============================================================================

std::vector<StmtPtr> ExternResolver::parseOsText(const std::string& osSource,
                                                   const std::string& syntheticFilename) const {
    if (osSource.empty()) return {};

    try {
        Lexer  lexer(osSource, syntheticFilename.c_str());
        Parser parser(lexer);
        auto   prog = parser.parse();

        // Move the statements out of the parsed program
        return std::move(prog->statements);

    } catch (const std::exception& e) {
        // Parse errors in generated code mean the emitter produced bad .os.
        // Report but don't crash the outer compilation.
        std::cerr << "[ExternResolver] ERROR parsing generated .os for '"
                  << syntheticFilename << "': " << e.what() << '\n';
        return {};
    }
}

// =============================================================================
// expandExtern — expand one ExternStmt into AST nodes
// =============================================================================

std::vector<StmtPtr> ExternResolver::expandExtern(const ExternStmt& stmt) {
    std::vector<StmtPtr> allNodes;

    for (const auto& hp : stmt.headerPaths) {
        // --- Step 1: find and read the header ---
        const std::string fullPath = findHeader(hp);
        if (fullPath.empty()) {
            std::cerr << "[ExternResolver] WARNING: header not found: " << hp << '\n';
            continue;
        }

        const std::string headerSrc = readFile(fullPath);
        if (headerSrc.empty()) {
            std::cerr << "[ExternResolver] WARNING: could not read: " << fullPath << '\n';
            continue;
        }

        // --- Step 1b: parse the C/C++ header ---
        CHeaderParser     cParser(headerSrc, fullPath);
        CHeaderResult     cResult = cParser.parse();

        if (!cResult.errors.empty()) {
            for (const auto& err : cResult.errors)
                std::cerr << "[ExternResolver] header warning: " << err << '\n';
        }

        // --- Step 2: emit to .os source text ---
        const std::string syntheticName = "<ffi:" + hp + ">";
        const std::string osText = emitToString(cResult, stmt.libraryPaths);

        // Optionally print for debugging
        if (m_cfg.debugPrint) {
            std::ostream& out = m_cfg.debugOut ? *m_cfg.debugOut : std::cout;
            out << "\n// " << std::string(72, '=') << '\n';
            out << "// [ExternResolver] generated .os for: " << hp << '\n';
            out << "// " << std::string(72, '=') << '\n';
            out << osText << '\n';
        }

        // --- Step 3: re-parse the .os text ---
        std::vector<StmtPtr> nodes = parseOsText(osText, syntheticName);

        // Accumulate
        for (auto& node : nodes)
            allNodes.push_back(std::move(node));
    }

    return allNodes;
}

// =============================================================================
// expand — mutate Program in-place
// =============================================================================

void ExternResolver::expand(Program& program) {
    // We walk statements and, for every ExternStmt with header paths,
    // generate new nodes and splice them in right after.
    //
    // We build a new list rather than inserting during iteration.

    std::vector<StmtPtr> result;
    result.reserve(program.statements.size() * 2);

    for (auto& stmtPtr : program.statements) {
        if (!stmtPtr) continue;

        if (stmtPtr->kind == NodeKind::ExternStmt) {
            const auto& ext = static_cast<const ExternStmt&>(*stmtPtr);

            // Keep the original ExternStmt (needed for linker info later)
            result.push_back(std::move(stmtPtr));

            if (!ext.headerPaths.empty()) {
                // Expand headers -> inject generated declarations
                std::vector<StmtPtr> generated = expandExtern(ext);
                for (auto& g : generated)
                    result.push_back(std::move(g));
            }
        } else {
            result.push_back(std::move(stmtPtr));
        }
    }

    program.statements = std::move(result);
}

} // namespace extern_support
} // namespace Omniscript