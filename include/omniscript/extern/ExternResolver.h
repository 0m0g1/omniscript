#pragma once
// engine/include/omniscript/extern/ExternResolver.h
//
// Expands ExternHeaderBlock statements into real declarations.
//
// Pipeline per header path:
//   ExternHeaderBlock.headerPaths
//       -> CHeaderParser   (preprocess + parse the C/C++ header)
//       -> OsEmitter       (flat .os source text)
//       -> Lexer + Parser  (re-parsed by the normal OmniScript parser)
//       -> vector<shared_ptr<Statement>>, spliced in right after the block
//
// Usage:
//   extern_support::ExternResolver resolver(cfg, sourceDir);
//   resolver.expand(statements);          // mutates in place; idempotent

#include <omniscript/extern/CHeaderParser.h>
#include <omniscript/extern/OsEmitter.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/ExternHeaderBlock.h>

#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Omniscript {
namespace extern_support {

struct ResolverConfig {
    // Extra header search directories (after the source file's own directory).
    std::vector<std::string> includeDirs;

    // Macros predefined for every header, e.g. { "_WIN32", "1" }.
    std::vector<std::pair<std::string, std::string>> defines;

    // Emitter options.
    EmitOptions emitOpts;

    // Print the generated .os text before re-parsing it.
    bool debugPrint = false;

    // Destination of the debug print (std::cout when null).
    std::ostream* debugOut = nullptr;
};

class ExternResolver {
public:
    // sourceDir: directory of the .os file being compiled (first header search location).
    ExternResolver(ResolverConfig cfg, std::string sourceDir);

    // Splices the generated declarations immediately after each unresolved
    // ExternHeaderBlock. The block is kept (and marked resolved), so calling
    // expand() again on the same statements does nothing.
    void expand(std::vector<std::shared_ptr<Statement>>& statements);

private:
    ResolverConfig m_cfg;
    std::string    m_sourceDir;
    OsEmitter      m_emitter;

    std::string findHeader(const std::string& hp) const;
    std::string readFile(const std::string& path) const;

    std::vector<std::shared_ptr<Statement>> expandOne(const ExternHeaderBlock& block);

    std::vector<std::shared_ptr<Statement>>
    parseOsText(const std::string& osSource,
                const std::string& syntheticFilename,
                const FileSpan& where) const;
};

} // namespace extern_support
} // namespace Omniscript