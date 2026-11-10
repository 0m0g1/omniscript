// src/Modules/ModuleResolver.cpp
#include <omniscript/Modules/ModuleResolver.h>
#include <omniscript/Modules/Sha256.h>
#include <omniscript/utils.h>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace Omniscript {

namespace fs = std::filesystem;

namespace {

bool startsWith(const std::string& s, const std::string& prefix) { return s.rfind(prefix, 0) == 0; }

std::string toLower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string dottedToPath(std::string s) {
    std::replace(s.begin(), s.end(), '.', '/');
    return s;
}

std::string stripOsExtension(const std::string& s) {
    if (s.size() > 3 && s.compare(s.size() - 3, 3, ".os") == 0) return s.substr(0, s.size() - 3);
    return s;
}

std::string hashContent(const std::string& text) { return "sha256-" + sha256Hex(text); }

bool readSource(const fs::path& p, std::string& out) {
    try { out = readFile(p.string()); return true; }
    catch (const std::exception&) { out.clear(); return false; }
}

// Returns the first candidate that is a regular file, logging every attempt for the error message.
std::optional<fs::path> firstExisting(const std::vector<fs::path>& candidates, std::vector<std::string>& tried) {
    for (const auto& c : candidates) {
        std::error_code ec;
        const bool ok = fs::is_regular_file(c, ec);
        tried.push_back("  " + c.generic_string() + (ok ? "  (found)" : ""));
        if (ok) return c;
    }
    return std::nullopt;
}

std::optional<ResolvedModule> load(const fs::path& file, const ModuleId& id, std::vector<std::string>& tried) {
    ResolvedModule m;
    if (!readSource(file, m.sourceText)) {
        tried.push_back("  " + file.generic_string() + "  (found but could not be read)");
        return std::nullopt;
    }
    m.id = id;
    m.displayPath = file.generic_string();
    m.contentHash = hashContent(m.sourceText);
    return m;
}

// <dir>/<rest>.os and <dir>/<rest>/mod.os (module with submodules)
void addModuleCandidates(std::vector<fs::path>& out, const fs::path& dir, const std::string& rest) {
    out.push_back(dir / (rest + ".os"));
    out.push_back(dir / rest / "mod.os");
}

fs::path importerDirOf(const ModuleRequest& req) {
    return req.importerFile.empty() ? fs::path() : fs::path(req.importerFile).parent_path();
}

// Explicit file imports: import "./helpers.os";
class PathSource : public ModuleSource {
public:
    std::string describe() const override { return "explicit path"; }
    std::optional<ResolvedModule> resolve(const ModuleRequest& req, std::vector<std::string>& tried) override {
        if (!req.isPath) return std::nullopt;
        std::vector<fs::path> candidates;
        const fs::path given(req.name);
        const fs::path dir = importerDirOf(req);
        candidates.push_back(given);
        if (!dir.empty() && given.is_relative()) candidates.push_back(dir / given);
        if (given.extension().empty()) {
            const size_t n = candidates.size();
            for (size_t i = 0; i < n; ++i) candidates.push_back(candidates[i].string() + ".os");
        }
        auto found = firstExisting(candidates, tried);
        if (!found) return std::nullopt;
        std::error_code ec;
        ModuleId id{"path", "", fs::weakly_canonical(*found, ec).generic_string()};
        return load(*found, id, tried);
    }
};

// std.* (logical) and std/* (legacy path form) -> <root>/standard/<edition>/...
class StdSource : public ModuleSource {
public:
    StdSource(std::string root, std::string edition) : root_(std::move(root)), edition_(std::move(edition)) {}
    std::string describe() const override { return "std (edition " + edition_ + ")"; }
    std::optional<ResolvedModule> resolve(const ModuleRequest& req, std::vector<std::string>& tried) override {
        std::string rest;
        if (req.isPath) {
            if (!startsWith(req.name, "std/") && !startsWith(req.name, "std\\")) return std::nullopt;
            rest = req.name.substr(4);
        } else if (req.name == "std") {
            rest = "std";
        } else if (startsWith(req.name, "std.")) {
            rest = dottedToPath(req.name.substr(4));
        } else {
            return std::nullopt;
        }
        rest = stripOsExtension(rest);

        std::vector<fs::path> roots{fs::path(root_)};
        const fs::path dir = importerDirOf(req);
        if (!dir.empty()) roots.push_back(dir);

        std::vector<std::string> spellings{rest};
        if (toLower(rest) != rest) spellings.push_back(toLower(rest)); // files are lowercase; tolerate Math.os for now

        std::vector<fs::path> candidates;
        for (const auto& r : roots)
            for (const auto& sp : spellings)
                addModuleCandidates(candidates, r / "standard" / edition_, sp);

        auto found = firstExisting(candidates, tried);
        if (!found) return std::nullopt;
        return load(*found, ModuleId{"std", edition_, rest}, tried);
    }
private:
    std::string root_, edition_;
};

// The current package's own modules: <manifest.name>.util -> <root>/src/util.os
class PackageSource : public ModuleSource {
public:
    explicit PackageSource(PackageManifest m) : manifest_(std::move(m)) {}
    std::string describe() const override { return "package '" + manifest_.name + "'"; }
    std::optional<ResolvedModule> resolve(const ModuleRequest& req, std::vector<std::string>& tried) override {
        if (req.isPath || manifest_.name.empty()) return std::nullopt;
        const std::string prefix = manifest_.name + ".";
        if (!startsWith(req.name, prefix)) return std::nullopt;
        const std::string rest = dottedToPath(req.name.substr(prefix.size()));
        std::vector<fs::path> candidates;
        addModuleCandidates(candidates, fs::path(manifest_.root) / "src", rest);
        auto found = firstExisting(candidates, tried);
        if (!found) return std::nullopt;
        return load(*found, ModuleId{manifest_.name, manifest_.version, rest}, tried);
    }
private:
    PackageManifest manifest_;
};

// Manifest dependencies. Only `path` dependencies work today; registry/url/git are reserved.
class DependencySource : public ModuleSource {
public:
    DependencySource(PackageManifest m, std::optional<Lockfile> l) : manifest_(std::move(m)), lock_(std::move(l)) {}
    std::string describe() const override { return "dependencies"; }
    std::optional<ResolvedModule> resolve(const ModuleRequest& req, std::vector<std::string>& tried) override {
        if (req.isPath) return std::nullopt;
        const auto dot = req.name.find('.');
        const std::string alias = req.name.substr(0, dot);
        auto it = manifest_.dependencies.find(alias);
        if (it == manifest_.dependencies.end()) return std::nullopt;
        const Dependency& dep = it->second;
        if (dep.kind != Dependency::Kind::Path) {
            tried.push_back("  dependency '" + alias + "': registry/url/git sources are not supported yet");
            return std::nullopt;
        }
        const std::string rest = dot == std::string::npos ? "mod" : dottedToPath(req.name.substr(dot + 1));
        std::vector<fs::path> candidates;
        addModuleCandidates(candidates, fs::path(manifest_.root) / dep.location / "src", rest);
        auto found = firstExisting(candidates, tried);
        if (!found) return std::nullopt;
        // TODO: read the dependency's own omni.json for its real name/version; verify integrity via lock_
        return load(*found, ModuleId{dep.package.empty() ? alias : dep.package, "path", rest}, tried);
    }
private:
    PackageManifest manifest_;
    std::optional<Lockfile> lock_;
};

std::unique_ptr<ModuleResolver>& globalSlot() {
    static std::unique_ptr<ModuleResolver> instance;
    return instance;
}

} // namespace

ModuleResolver::ModuleResolver(std::string toolchainRoot,
                               std::optional<PackageManifest> manifest,
                               std::optional<Lockfile> lock) {
    const std::string edition = manifest ? manifest->edition : "1";
    sources_.push_back(std::make_unique<PathSource>());
    sources_.push_back(std::make_unique<StdSource>(std::move(toolchainRoot), edition)); // before packages: std can't be shadowed
    if (manifest) {
        sources_.push_back(std::make_unique<PackageSource>(*manifest));
        sources_.push_back(std::make_unique<DependencySource>(*manifest, std::move(lock)));
    }
}

void ModuleResolver::configureGlobal(std::string toolchainRoot,
                                     std::optional<PackageManifest> manifest,
                                     std::optional<Lockfile> lock) {
    globalSlot() = std::make_unique<ModuleResolver>(std::move(toolchainRoot), std::move(manifest), std::move(lock));
}

ModuleResolver& ModuleResolver::global() {
    if (!globalSlot()) {
        std::string error;
        std::error_code ec;
        auto manifest = findManifest(fs::current_path(ec).string(), error);
        configureGlobal(".", std::move(manifest), std::nullopt);
    }
    return *globalSlot();
}

ResolveResult ModuleResolver::resolve(const ModuleRequest& req) {
    ResolveResult result;
    for (auto& src : sources_) {
        result.tried.push_back("[" + src->describe() + "]");
        if (auto m = src->resolve(req, result.tried)) {
            result.module = std::move(m);
            return result;
        }
    }
    result.error = "Module '" + req.name + "' not found.";
    return result;
}

bool ModuleResolver::beginLoad(const ModuleId& id) { return loading_.insert(id.toString()).second; }
void ModuleResolver::endLoad(const ModuleId& id) { loading_.erase(id.toString()); }

} // namespace Omniscript