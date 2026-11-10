// include/omniscript/Modules/ModuleResolver.h
#pragma once
#include <omniscript/Modules/ModuleSource.h>
#include <omniscript/Modules/PackageManifest.h>
#include <omniscript/Modules/Lockfile.h>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace Omniscript {

struct ResolveResult {
    std::optional<ResolvedModule> module;
    std::vector<std::string> tried;  // human-readable attempts, used in "not found" errors
    std::string error;               // empty on success
};

class ModuleResolver {
public:
    // manifest/lock are null in single-file script mode (std + path imports only).
    ModuleResolver(std::string toolchainRoot,
                   std::optional<PackageManifest> manifest,
                   std::optional<Lockfile> lock);

    // Process-wide resolver used by ImportModule::express(). Lazily created in script mode
    // (toolchain root ".", manifest found by walking up from the cwd). The driver can call
    // configureGlobal() first to set the real toolchain root / manifest.
    static ModuleResolver& global();
    static void configureGlobal(std::string toolchainRoot,
                                std::optional<PackageManifest> manifest,
                                std::optional<Lockfile> lock);

    // Order: path (if isPath) -> std -> own package -> dependencies.
    // std is checked first for "std.*" so a dependency can't shadow it.
    ResolveResult resolve(const ModuleRequest& req);

    // Cycle detection by canonical ID. Returns false if `id` is already loading.
    bool beginLoad(const ModuleId& id);
    void endLoad(const ModuleId& id);

    // Later: bytecode cache keyed by (ModuleId, contentHash).
    // std::optional<CachedModule> lookupCache(const ModuleId&, const std::string& hash);

private:
    std::vector<std::unique_ptr<ModuleSource>> sources_;
    std::unordered_set<std::string> loading_;
};

} // namespace Omniscript