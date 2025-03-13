#include <omniscript/runtime/Path/Path.h>
#include <omniscript/omniscript_pch.h>
#include <filesystem>

Path::Path() {
    registerMethods();
}

void Path::registerMethods() {
    addMethod("join", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return join(std::get<std::vector<std::string>>(args[0]));
    });

    addMethod("dirname", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return dirname(std::get<std::string>(args[0]));
    });

    addMethod("basename", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        std::string path = std::get<std::string>(args[0]);
        std::string ext = args.size() > 1 ? std::get<std::string>(args[1]) : "";
        return basename(path, ext);
    });

    addMethod("extname", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return extname(std::get<std::string>(args[0]));
    });

    addMethod("resolve", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return resolve(std::get<std::vector<std::string>>(args[0]));
    });

    addMethod("normalize", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return normalize(std::get<std::string>(args[0]));
    });

    addMethod("isAbsolute", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return isAbsolute(std::get<std::string>(args[0]));
    });
}

std::string Path::join(const std::vector<std::string>& paths) {
    std::filesystem::path result;
    for (const auto& path : paths) {
        result /= path;
    }
    return result.lexically_normal().string();
}

std::string Path::dirname(const std::string& path) {
    return std::filesystem::path(path).parent_path().string();
}

std::string Path::basename(const std::string& path, const std::string& ext) {
    std::string base = std::filesystem::path(path).filename().string();
    if (!ext.empty() && base.ends_with(ext)) {
        return base.substr(0, base.size() - ext.size());
    }
    return base;
}

std::string Path::extname(const std::string& path) {
    return std::filesystem::path(path).extension().string();
}

std::string Path::resolve(const std::vector<std::string>& paths) {
    std::filesystem::path result = std::filesystem::current_path();
    for (const auto& path : paths) {
        result /= path;
    }
    return result.lexically_normal().string();
}

std::string Path::normalize(const std::string& path) {
    return std::filesystem::path(path).lexically_normal().string();
}

bool Path::isAbsolute(const std::string& path) {
    return std::filesystem::path(path).is_absolute();
}

std::string Path::removeTrailingSlashes(const std::string& path) {
    std::string normalized = path;
    while (!normalized.empty() && (normalized.back() == '/' || normalized.back() == '\\')) {
        normalized.pop_back();
    }
    return normalized;
}