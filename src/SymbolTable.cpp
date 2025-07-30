#include <omniscript/SymbolTable.h>
#include <omniscript/utils.h>
#include <omniscript/Console.h>
#include <algorithm>
#include <sstream>
#include <queue>
#include <regex>
#include <iomanip>

namespace Omniscript {

// Static member initialization
template <typename T, typename TypeT>
std::unordered_map<std::string, ModuleInfo<T, TypeT>> SymbolTable<T, TypeT>::globalModules_;

template <typename T, typename TypeT>
std::mutex SymbolTable<T, TypeT>::modulesMutex_;

// ==================== SYMBOLTABLEUTILS IMPLEMENTATIONS ====================
std::string SymbolTableUtils::normalizePath(const std::string& path) {
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    while (!result.empty() && result.back() == '/') {
        result.pop_back();
    }
    return result;
}

std::string SymbolTableUtils::resolvePath(const std::string& basePath, const std::string& relativePath) {
    std::vector<std::string> baseParts = splitPath(basePath);
    std::vector<std::string> relParts = splitPath(relativePath);
    
    if (!relativePath.empty() && relativePath[0] == '/') {
        return normalizePath(relativePath);
    }
    
    baseParts.pop_back(); // Remove file name if present
    baseParts.insert(baseParts.end(), relParts.begin(), relParts.end());
    
    std::vector<std::string> result;
    for (const auto& part : baseParts) {
        if (part == ".") {
            continue;
        }
        if (part == ".." && !result.empty()) {
            result.pop_back();
        } else if (part != "..") {
            result.push_back(part);
        }
    }
    
    return joinPath(result);
}

bool SymbolTableUtils::isValidModuleName(const std::string& name) {
    if (name.empty()) return false;
    std::regex validName("^[a-zA-Z_][a-zA-Z0-9_]*(/[a-zA-Z_][a-zA-Z0-9_]*)*$");
    return std::regex_match(name, validName);
}

std::vector<std::string> SymbolTableUtils::splitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string normalized = normalizePath(path);
    std::stringstream ss(normalized);
    std::string part;
    while (std::getline(ss, part, '/')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return parts;
}

std::string SymbolTableUtils::joinPath(const std::vector<std::string>& components) {
    if (components.empty()) return "";
    std::stringstream ss;
    for (size_t i = 0; i < components.size(); ++i) {
        ss << components[i];
        if (i < components.size() - 1) {
            ss << "/";
        }
    }
    return ss.str();
}

} // namespace Omniscript

// Explicit template instantiations (if needed)
// template class Omniscript::SymbolTable<int, void>;
// template class Omniscript::SymbolTable<std::string, void>;