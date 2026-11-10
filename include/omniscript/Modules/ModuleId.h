// include/omniscript/Modules/ModuleId.h
#pragma once
#include <cctype>
#include <cstdio>
#include <string>

namespace Omniscript {

// Canonical identity of a loaded module. The symbol-table registry,
// cycle detection and the module cache are all keyed on this.
struct ModuleId {
    std::string package;     // "std", "acme.json", or the root package name
    std::string version;     // "1" for std (edition), "2.1.0" for packages
    std::string modulePath;  // "json", "net/http" (slashes, no extension)

    std::string toString() const { return package + "@" + version + "/" + modulePath; }

    // Key for SymbolTable::defineModule(). The table only accepts `ident(/ident)*` names, so
    // '@', '.', '-' and leading digits are rewritten: std@1/json -> "std/v1/json",
    // acme.json@2.1.0/net/http -> "acme_json/v2_1_0/net/http". Path imports use a hash of the file.
    std::string registryKey() const {
        auto ident = [](const std::string& s) {
            std::string out;
            for (char c : s) out += (std::isalnum(static_cast<unsigned char>(c)) || c == '_') ? c : '_';
            if (out.empty() || std::isdigit(static_cast<unsigned char>(out[0]))) out.insert(out.begin(), '_');
            return out;
        };
        if (package == "path") {
            unsigned long long h = 1469598103934665603ULL;
            for (unsigned char c : modulePath) { h ^= c; h *= 1099511628211ULL; }
            char buf[24];
            std::snprintf(buf, sizeof(buf), "path/h%016llx", h);
            return buf;
        }
        std::string key = ident(package) + "/v";
        for (char c : version) key += (std::isalnum(static_cast<unsigned char>(c)) || c == '_') ? c : '_';
        size_t start = 0;
        while (start <= modulePath.size()) {
            size_t end = modulePath.find('/', start);
            if (end == std::string::npos) end = modulePath.size();
            if (end > start) key += "/" + ident(modulePath.substr(start, end - start));
            start = end + 1;
        }
        return key;
    }
    bool operator==(const ModuleId& o) const {
        return package == o.package && version == o.version && modulePath == o.modulePath;
    }
};

} // namespace Omniscript

namespace std {
template <> struct hash<Omniscript::ModuleId> {
    size_t operator()(const Omniscript::ModuleId& id) const {
        return hash<string>()(id.toString());
    }
};
}