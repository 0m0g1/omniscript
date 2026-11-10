// engine/src/omniscript/extern/ExternResolver.cpp
//
// Expands ExternHeaderBlock statements into real OmniScript declarations.

#include <omniscript/extern/ExternResolver.h>
#include <omniscript/Lexer.h>
#include <omniscript/Parser.h>

#include <algorithm>
#include <cctype>
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
    std::error_code ec;

    // 1. absolute path
    {
        fs::path p(hp);
        if (p.is_absolute() && fs::exists(p, ec)) return p.string();
    }
    // 2. relative to the .os source file
    {
        fs::path p = fs::path(m_sourceDir) / hp;
        if (fs::exists(p, ec)) return p.lexically_normal().string();
    }
    // 3. extra include directories, in order
    for (const auto& dir : m_cfg.includeDirs) {
        fs::path p = fs::path(dir) / hp;
        if (fs::exists(p, ec)) return p.lexically_normal().string();
    }
    // 4. relative to the working directory (project root)
    {
        fs::path cwd = fs::current_path(ec);
        if (!ec) {
            fs::path p = cwd / hp;
            if (fs::exists(p, ec)) return p.lexically_normal().string();
        }
    }
    // 5. anywhere under <cwd>/dependencies, matching the trailing path (e.g. "GLFW/glfw3.h" or "glfw3.h")
    {
        auto lower = [](std::string v) {
            std::transform(v.begin(), v.end(), v.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return v;
        };
        const fs::path root = fs::current_path(ec) / "dependencies";
        const std::string wanted = lower(fs::path(hp).lexically_normal().generic_string());
        if (!wanted.empty() && fs::is_directory(root, ec)) {
            std::string best;
            fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
            for (; !ec && it != end; it.increment(ec)) {
                std::error_code fec;
                if (!it->is_regular_file(fec)) continue;
                const std::string cand = lower(it->path().generic_string());
                const bool endsWith = cand.size() >= wanted.size() &&
                    cand.compare(cand.size() - wanted.size(), wanted.size(), wanted) == 0 &&
                    (cand.size() == wanted.size() || cand[cand.size() - wanted.size() - 1] == '/');
                if (endsWith) {
                    std::string full = fs::absolute(it->path(), fec).string();
                    if (best.empty() || full.size() < best.size()) best = full;
                }
            }
            if (!best.empty()) return best;
        }
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
// Generated .os text -> statements
// =============================================================================

std::vector<std::shared_ptr<Statement>>
ExternResolver::parseOsText(const std::string& osSource,
                            const std::string& syntheticFilename,
                            const FileSpan& where) const {
    if (osSource.empty()) return {};

    try {
        Lexer  lexer(osSource, syntheticFilename.c_str());
        Parser parser(lexer);
        return parser.parse();
    } catch (const std::exception& e) {
        // The emitter produced something the parser rejects. Report it; do not
        // let an exception escape (callers are noexcept).
        console.reportError(
            Console::INTERNAL_ERROR,
            "Failed to parse OmniScript generated from header '" + syntheticFilename + "': " + e.what(),
            "To resolve this:\n1. Re-run with --debug to print the generated declarations\n"
            "2. Check the header for constructs the FFI emitter does not support",
            where
        );
        return {};
    }
}

// =============================================================================
// expandOne
// =============================================================================

std::vector<std::shared_ptr<Statement>>
ExternResolver::expandOne(const ExternHeaderBlock& block) {
    std::vector<std::shared_ptr<Statement>> out;

    for (const auto& hp : block.headerPaths) {
        const std::string fullPath = findHeader(hp);
        if (fullPath.empty()) {
            console.reportError(
                Console::INTERNAL_ERROR,
                "Header not found: '" + hp + "'",
                "To resolve this:\n1. Check the path in the extern declaration\n"
                "2. Use a path relative to the .os file or an absolute path",
                block.getSpan()
            );
            continue;
        }

        const std::string headerSrc = readFile(fullPath);
        if (headerSrc.empty()) {
            console.reportError(
                Console::INTERNAL_ERROR,
                "Could not read header: '" + fullPath + "'",
                "To resolve this:\n1. Check file permissions\n2. Make sure the header is not empty",
                block.getSpan()
            );
            continue;
        }

        CHeaderParser cParser(headerSrc, fullPath);
        for (const auto& d : m_cfg.defines) cParser.addDefine(d.first, d.second);
        CHeaderResult cResult = cParser.parse();

        for (const auto& err : cResult.errors)
            { std::ostringstream ws; ws << "[ExternResolver] header warning (" << hp << "): " << err; console.reportWarning(ws.str(), FileSpan{}); }

        std::ostringstream oss;
        m_emitter.emit(oss, cResult, block.libraryPaths);
        const std::string osText = oss.str();

        if (m_cfg.debugPrint) {
            std::ostream& dbg = m_cfg.debugOut ? *m_cfg.debugOut : std::cout;
            dbg << "\n// " << std::string(72, '=') << '\n'
                << "// [ExternResolver] generated .os for: " << hp << '\n'
                << "// " << std::string(72, '=') << '\n'
                << osText << '\n';
        }

        auto nodes = parseOsText(osText, "<ffi:" + hp + ">", block.getSpan());
        for (auto& n : nodes) out.push_back(std::move(n));
    }
    return out;
}

// =============================================================================
// expand
// =============================================================================

void ExternResolver::expand(std::vector<std::shared_ptr<Statement>>& statements) {
    std::vector<std::shared_ptr<Statement>> result;
    result.reserve(statements.size() * 2);

    for (auto& stmt : statements) {
        if (!stmt) continue;

        auto block = std::dynamic_pointer_cast<ExternHeaderBlock>(stmt);
        if (block && !block->resolved) {
            block->resolved = true;          // set first: never expand twice
            result.push_back(stmt);          // keep the block itself
            auto generated = expandOne(*block);
            for (auto& g : generated) result.push_back(std::move(g));
        } else {
            result.push_back(std::move(stmt));
        }
    }
    statements = std::move(result);
}

} // namespace extern_support
} // namespace Omniscript