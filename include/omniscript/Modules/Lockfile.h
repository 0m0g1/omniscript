// include/omniscript/Modules/Lockfile.h
#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace Omniscript {

// omni.lock.json - machine-written, sorted keys, committed to VCS.
// { "lockVersion": 1,
//   "packages": { "acme.json@2.1.0": {
//       "source": "registry+https://pkgs.example.org", "integrity": "sha256-...",
//       "dependencies": ["acme.util@1.4.2"] } } }
struct LockedPackage {
    std::string name;
    std::string version;
    std::string source;                     // "registry+<url>", "path+<dir>", "url+<url>", "git+<url>#<rev>"
    std::string integrity;                  // verified on every load
    std::vector<std::string> dependencies;  // "name@version"
};

struct Lockfile {
    int lockVersion = 1;
    std::map<std::string, LockedPackage> packages;  // key "name@version"

    static std::optional<Lockfile> read(const std::string& path, std::string& error);
    bool write(const std::string& path, std::string& error) const;  // sorted, stable output
};

} // namespace Omniscript
