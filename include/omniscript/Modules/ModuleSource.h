// include/omniscript/Modules/ModuleSource.h
#pragma once
#include <omniscript/Modules/ModuleId.h>
#include <optional>
#include <string>
#include <vector>

namespace Omniscript {

// What an import statement asks for.
struct ModuleRequest {
    std::string name;            // "std.json" (dotted) or "./helpers.os" (path)
    bool isPath = false;         // true => explicit file import
    ModuleId importer;           // who is asking (own-package lookup)
    std::string importerFile;    // file containing the import (for relative paths)
};

struct ResolvedModule {
    ModuleId id;
    std::string sourceText;      // or empty if served from a bytecode cache
    std::string displayPath;     // for diagnostics only
    std::string contentHash;     // sha256 of source; cache key + lockfile integrity
};

// One place modules can come from. Resolver tries sources in order.
class ModuleSource {
public:
    virtual ~ModuleSource() = default;
    virtual std::string describe() const = 0;  // "std (edition 1)", "package 'app'", ...
    // Return nullopt if this source doesn't have it. Append what was tried to `tried`.
    virtual std::optional<ResolvedModule> resolve(const ModuleRequest& req,
                                                  std::vector<std::string>& tried) = 0;
};

// Concrete sources (stubs live in ModuleResolver.cpp):
//   PathSource        - explicit "./x.os" imports
//   PackageSource     - current package: app.util -> <root>/src/util.os
//   DependencySource  - manifest/lockfile deps (registry, path, url, git)
//   StdSource         - "std.*" -> <toolchain>/standard/<edition>/<name>.os; reserved, can't be shadowed

} // namespace Omniscript