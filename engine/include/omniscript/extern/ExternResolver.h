#pragma once
// engine/include/omniscript/extern/ExternResolver.h
//
// Expands ExternStmt nodes that carry header paths into real AST declarations.
//
// Pipeline per extern header:
//   ExternStmt.headerPaths
//       -> CHeaderParser   (parses the C/C++ header)
//       -> OsEmitter       (converts to .os source text)
//       -> Lexer + Parser  (re-parses the .os text through YOUR existing parser)
//       -> vector<StmtPtr> (FunctionDeclStmt / VarDeclStmt / etc.)
//
// The resulting statements are injected directly after the originating
// ExternStmt in the Program's statement list.
//
// Usage:
//   extern_support::ExternResolver resolver(cfg, sourceDir);
//   resolver.expand(*program);   // mutates program in-place

#include <omniscript/extern/CHeaderParser.h>
#include <omniscript/extern/OsEmitter.h>
#include <omniscript/ast/Ast.h>
#include <omniscript/ast/AstStatement.h>

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace Omniscript {
namespace extern_support {

// -----------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------
struct ResolverConfig {
    // Directories to search for headers, in priority order.
    // The source file's own directory is always searched first.
    std::vector<std::string> includeDirs;

    // Emitter options (comments, skip private names, etc.)
    EmitOptions emitOpts;

    // When true, also print the generated .os text to `debugOut`
    // before parsing it — useful during development.
    bool debugPrint = true;

    // Stream to print debug output to (ignored when debugPrint=false)
    std::ostream* debugOut = nullptr;
};

// -----------------------------------------------------------------------
// ExternResolver
// -----------------------------------------------------------------------
class ExternResolver {
public:
    // sourceDir: directory of the .os source file being compiled.
    //            Used as the first search location for headers.
    ExternResolver(ResolverConfig cfg, std::string sourceDir);

    // Walk `program`, find every ExternStmt with header paths, expand it
    // into real AST nodes, and inject them into program.statements in-place.
    //
    // After this call, program.statements contains:
    //   ... ExternStmt(headerPaths=[...]) ...   <- original, kept
    //   ... FunctionDeclStmt(add_i32) ...       <- injected
    //   ... FunctionDeclStmt(fill_buffer) ...   <- injected
    //   ... (etc.)
    void expand(Program& program);

    // Expand a single ExternStmt.
    // Returns the new AST nodes to inject (does NOT modify program itself).
    std::vector<StmtPtr> expandExtern(const ExternStmt& stmt);

private:
    ResolverConfig m_cfg;
    std::string    m_sourceDir;
    OsEmitter      m_emitter;

    // Resolve a header path to an absolute path. Returns "" if not found.
    std::string findHeader(const std::string& hp) const;

    // Read a file into a string. Returns "" on failure.
    std::string readFile(const std::string& path) const;

    // Emit CHeaderResult -> .os source text
    std::string emitToString(const CHeaderResult& result,
                             const std::vector<std::string>& libPaths) const;

    // Parse .os source text -> AST statement list
    // `syntheticFilename` is used in error messages.
    std::vector<StmtPtr> parseOsText(const std::string& osSource,
                                     const std::string& syntheticFilename) const;
};

} // namespace extern_support
} // namespace Omniscript