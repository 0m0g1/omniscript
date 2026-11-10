// include/omniscript/Modules/PackageManifest.h
#pragma once
#include <map>
#include <optional>
#include <string>

namespace Omniscript {

struct Dependency {
    enum class Kind { Registry, Path, Url, Git };
    std::string alias;       // name used in import statements: "json2"
    std::string package;     // real package name; defaults to alias
    std::string version;     // semver range for Registry: "^2.1"
    Kind kind = Kind::Registry;
    std::string location;    // path / url / git url (unused for Registry)
    std::string integrity;   // "sha256-..." required for Url, optional otherwise
};

// omni.json (JSONC allowed):
// { "name": "wordtop", "version": "0.3.0", "edition": "1",
//   "main": "src/main.os",
//   "dependencies": { "json2": { "package": "acme.json", "version": "^2" } },
//   "devDependencies": {} }
struct PackageManifest {
    std::string name;
    std::string version;
    std::string edition = "1";            // selects standard/<edition>/
    std::string main = "src/main.os";
    std::string root;                     // directory containing the manifest
    std::map<std::string, Dependency> dependencies;
    std::map<std::string, Dependency> devDependencies;
};

// Format-agnostic so other formats can be added later without touching callers.
class ManifestReader {
public:
    virtual ~ManifestReader() = default;
    virtual bool canRead(const std::string& filename) const = 0;  // "omni.json"
    virtual std::optional<PackageManifest> read(const std::string& path, std::string& error) = 0;
};

// Walks up from `startDir` looking for omni.json. nullopt => single-file script mode.
std::optional<PackageManifest> findManifest(const std::string& startDir, std::string& error);

} // namespace Omniscript
