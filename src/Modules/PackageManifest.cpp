// src/Modules/PackageManifest.cpp
#include <omniscript/Modules/PackageManifest.h>

namespace Omniscript {

std::optional<PackageManifest> findManifest(const std::string& startDir, std::string& error) {
    // TODO: walk up from startDir looking for omni.json, parse it with a JSONC-capable reader
    //       (comments + trailing commas), fill PackageManifest::root with the directory found.
    // Until then every program runs in single-file script mode: std + explicit path imports only.
    (void)startDir; (void)error;
    return std::nullopt;
}

} // namespace Omniscript