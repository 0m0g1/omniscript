#pragma once

#include <omniscript/omniscript_pch.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <queue>
#include <regex>
#include <iomanip>

namespace Omniscript {

// Forward declarations
template <typename T, typename TypeT>
class SymbolTable;

template <typename T, typename TypeT>
class ModuleManager;

// Symbol information structure
template <typename T>
struct SymbolInfo {
    T value;
    std::string name;
    std::string type;
    bool isConstant = false;
    bool isPublic = true;
    bool isStatic = false;
    size_t line = 0;
    size_t column = 0;
    std::string filePath;
    std::string documentation;
    
    SymbolInfo() = default;
    SymbolInfo(T val, const std::string& n, bool constant = false, bool pub = true)
        : value(std::move(val)), name(n), isConstant(constant), isPublic(pub) {}
    SymbolInfo(T val, const std::string& n, const std::string& t, bool constant = false, bool pub = true)
        : value(std::move(val)), name(n), type(t), isConstant(constant), isPublic(pub) {}
};

// Overload information for functions
template <typename T>
struct OverloadInfo {
    std::vector<T> overloads;
    std::string name;
    bool isPublic = true;
    
    OverloadInfo() = default;
    explicit OverloadInfo(const std::string& n, bool pub = true) 
        : name(n), isPublic(pub) {}
    
    void addOverload(T overload) {
        overloads.push_back(std::move(overload));
    }
    
    size_t getOverloadCount() const { return overloads.size(); }
    bool hasOverloads() const { return !overloads.empty(); }
};

// Module information structure
template <typename T, typename TypeT = void>
struct ModuleInfo {
    std::string name;
    std::string fullPath;
    std::string version;
    std::shared_ptr<SymbolTable<T, TypeT>> symbolTable;
    std::unordered_set<std::string> dependencies;
    std::unordered_set<std::string> exports;
    bool isLoaded = false;
    bool isSystem = false;
    
    ModuleInfo() = default;
    ModuleInfo(const std::string& n, const std::string& path, const std::string& ver,
           std::shared_ptr<SymbolTable<T, TypeT>> table)
        : name(n), fullPath(path), version(ver), symbolTable(std::move(table)) {}
};

// Symbol lookup result
template <typename T, typename TypeT = void>
struct LookupResult {
    enum class Type { NotFound, Variable, Constant, Overload, Module, Type };

    Type type = Type::NotFound;
    T value = {};
    std::vector<T> overloads;
    std::string name;
    bool isConstant = false;
    bool isPublic = true;

    std::shared_ptr<SymbolTable<T, TypeT>> scope;

    bool found() const { return type != Type::NotFound; }
    bool isVariable() const { return type == Type::Variable; }
    bool isConstantValue() const { return type == Type::Constant; }
    bool isOverloadSet() const { return type == Type::Overload; }
    bool isModuleRef() const { return type == Type::Module; }
    bool isTypeRef() const { return type == Type::Type; }
};

// Utility functions and type traits
namespace SymbolTableUtils {
    inline std::string normalizePath(const std::string& path) {
        std::string result = path;
        std::replace(result.begin(), result.end(), '\\', '/');
        while (!result.empty() && result.back() == '/') {
            result.pop_back();
        }
        return result;
    }

    inline std::vector<std::string> splitPath(const std::string& path) {
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

    inline std::string joinPath(const std::vector<std::string>& components) {
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

    inline std::string resolvePath(const std::string& basePath, const std::string& relativePath) {
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

    inline bool isValidModuleName(const std::string& name) {
        if (name.empty()) return false;
        std::regex validName("^[a-zA-Z_][a-zA-Z0-9_]*(/[a-zA-Z_][a-zA-Z0-9_]*)*$");
        return std::regex_match(name, validName);
    }
}

// Enhanced SymbolTable class with better module support and thread safety
template <typename T, typename TypeT = void>
class SymbolTable : public std::enable_shared_from_this<SymbolTable<T, TypeT>> {
public:
    // Type aliases for convenience
    using SymbolInfoType = SymbolInfo<T>;
    using OverloadInfoType = OverloadInfo<T>;
    using ModuleInfoType = ModuleInfo<T, TypeT>;
    using LookupResultType = LookupResult<T>;
    using SymbolTablePtr = std::shared_ptr<SymbolTable<T, TypeT>>;
    using WeakSymbolTablePtr = std::weak_ptr<SymbolTable<T, TypeT>>;
    using ValidationFunction = std::function<bool(const std::string&, const T&)>;
    
    // Event callback types
    using SymbolCallback = std::function<void(const std::string&, const T&)>;
    using ScopeCallback = std::function<void(const std::string&)>;

    // Constructor and destructor
    explicit SymbolTable(SymbolTablePtr parent = nullptr, const std::string& name = "")
        : name_(name), parent_(parent), threadSafeMode_(false) {}
    virtual ~SymbolTable() = default;

    // Disable copy constructor and assignment (use shared_ptr)
    SymbolTable(const SymbolTable&) = delete;
    SymbolTable& operator=(const SymbolTable&) = delete;

    // Enable move constructor and assignment
    SymbolTable(SymbolTable&&) = default;
    SymbolTable& operator=(SymbolTable&&) = default;

    // ==================== BASIC VALUE MANAGEMENT ====================
    void set(const std::string& name, T value, bool isPublic = true) {
        setVariable(name, std::move(value), isPublic);
    }

    T get(const std::string& name) const {
        return getValue(name);
    }

    bool exists(const std::string& name) const {
        return lookup(name).found();
    }
    
    bool setVariable(const std::string& name, T value, bool isPublic = true) {
        return threadSafeMode_ ? withWriteLock([&]() {
            return setSymbolInternal(name, std::move(value), false, isPublic);
        }) : setSymbolInternal(name, std::move(value), false, isPublic);
    }

    bool setConstant(const std::string& name, T value, bool isPublic = true) {
        return threadSafeMode_ ? withWriteLock([&]() {
            return setSymbolInternal(name, std::move(value), true, isPublic);
        }) : setSymbolInternal(name, std::move(value), true, isPublic);
    }

    bool addOverloadable(const std::string& name, T value, bool isPublic = true) {
        return threadSafeMode_ ? withWriteLock([&]() {
            overloads_[name].addOverload(std::move(value));
            overloads_[name].isPublic = isPublic;
            notifySymbolChanged(name, value);
            return true;
        }) : [&]() {
            overloads_[name].addOverload(std::move(value));
            overloads_[name].isPublic = isPublic;
            notifySymbolChanged(name, value);
            return true;
        }();
    }

    bool removeSymbol(const std::string& name) {
        return threadSafeMode_ ? withWriteLock([&]() {
            bool removed = symbols_.erase(name) > 0 || overloads_.erase(name) > 0;
            if (removed) notifySymbolChanged(name, T{});
            return removed;
        }) : [&]() {
            bool removed = symbols_.erase(name) > 0 || overloads_.erase(name) > 0;
            if (removed) notifySymbolChanged(name, T{});
            return removed;
        }();
    }

    bool updateVariable(const std::string& name, T newValue) {
        return threadSafeMode_ ? withWriteLock([&]() {
            auto it = symbols_.find(name);
            if (it != symbols_.end() && !it->second.isConstant) {
                it->second.value = std::move(newValue);
                notifySymbolChanged(name, newValue);
                return true;
            }
            return false;
        }) : [&]() {
            auto it = symbols_.find(name);
            if (it != symbols_.end() && !it->second.isConstant) {
                it->second.value = std::move(newValue);
                notifySymbolChanged(name, newValue);
                return true;
            }
            return false;
        }();
    }

    T getValue(const std::string& name) const {
        auto result = lookup(name);
        if (result.found()) {
            return result.value;
        }
        throw std::runtime_error("Symbol not found: " + name);
    }

    T* getPointerToValue(const std::string& name) {
        return threadSafeMode_ ? withWriteLock([&]() -> T* {
            auto it = symbols_.find(name);
            return it != symbols_.end() ? &it->second.value : nullptr;
        }) : [&]() -> T* {
            auto it = symbols_.find(name);
            return it != symbols_.end() ? &it->second.value : nullptr;
        }();
    }

    const T* getConstPointerToValue(const std::string& name) const {
        return threadSafeMode_ ? withReadLock([&]() -> const T* {
            auto it = symbols_.find(name);
            return it != symbols_.end() ? &it->second.value : nullptr;
        }) : [&]() -> const T* {
            auto it = symbols_.find(name);
            return it != symbols_.end() ? &it->second.value : nullptr;
        }();
    }

    std::vector<T> getOverloads(const std::string& name) const {
        return threadSafeMode_ ? withReadLock([&]() {
            auto it = overloads_.find(name);
            if (it != overloads_.end()) {
                return it->second.overloads;
            }
            auto parentPtr = parent_.lock();
            return parentPtr ? parentPtr->getOverloads(name) : std::vector<T>{};
        }) : [&]() {
            auto it = overloads_.find(name);
            if (it != overloads_.end()) {
                return it->second.overloads;
            }
            auto parentPtr = parent_.lock();
            return parentPtr ? parentPtr->getOverloads(name) : std::vector<T>{};
        }();
    }

    LookupResultType lookup(const std::string& name) const {
        return lookupInternal(name, true);
    }

    std::optional<SymbolInfoType> getSymbolInfo(const std::string& name) const {
        return threadSafeMode_ ? withReadLock([&]() -> std::optional<SymbolInfoType> {
            auto it = symbols_.find(name);
            if (it != symbols_.end()) {
                return it->second;
            }
            auto parentPtr = parent_.lock();
            return parentPtr ? parentPtr->getSymbolInfo(name) : std::nullopt;
        }) : [&]() -> std::optional<SymbolInfoType> {
            auto it = symbols_.find(name);
            if (it != symbols_.end()) {
                return it->second;
            }
            auto parentPtr = parent_.lock();
            return parentPtr ? parentPtr->getSymbolInfo(name) : std::nullopt;
        }();
    }

    std::vector<std::string> getAllSymbolNames(bool includeParent = true) const {
        return threadSafeMode_ ? withReadLock([&]() {
            std::vector<std::string> names;
            for (const auto& [name, _] : symbols_) {
                names.push_back(name);
            }
            for (const auto& [name, _] : overloads_) {
                names.push_back(name);
            }
            if (includeParent) {
                auto parentPtr = parent_.lock();
                if (parentPtr) {
                    auto parentNames = parentPtr->getAllSymbolNames(true);
                    names.insert(names.end(), parentNames.begin(), parentNames.end());
                }
            }
            return names;
        }) : [&]() {
            std::vector<std::string> names;
            for (const auto& [name, _] : symbols_) {
                names.push_back(name);
            }
            for (const auto& [name, _] : overloads_) {
                names.push_back(name);
            }
            if (includeParent) {
                auto parentPtr = parent_.lock();
                if (parentPtr) {
                    auto parentNames = parentPtr->getAllSymbolNames(true);
                    names.insert(names.end(), parentNames.begin(), parentNames.end());
                }
            }
            return names;
        }();
    }

    std::vector<std::string> getPublicSymbolNames() const {
        return threadSafeMode_ ? withReadLock([&]() {
            std::vector<std::string> names;
            for (const auto& [name, info] : symbols_) {
                if (info.isPublic) names.push_back(name);
            }
            for (const auto& [name, info] : overloads_) {
                if (info.isPublic) names.push_back(name);
            }
            return names;
        }) : [&]() {
            std::vector<std::string> names;
            for (const auto& [name, info] : symbols_) {
                if (info.isPublic) names.push_back(name);
            }
            for (const auto& [name, info] : overloads_) {
                if (info.isPublic) names.push_back(name);
            }
            return names;
        }();
    }

    std::vector<std::string> getPrivateSymbolNames() const {
        return threadSafeMode_ ? withReadLock([&]() {
            std::vector<std::string> names;
            for (const auto& [name, info] : symbols_) {
                if (!info.isPublic) names.push_back(name);
            }
            for (const auto& [name, info] : overloads_) {
                if (!info.isPublic) names.push_back(name);
            }
            return names;
        }) : [&]() {
            std::vector<std::string> names;
            for (const auto& [name, info] : symbols_) {
                if (!info.isPublic) names.push_back(name);
            }
            for (const auto& [name, info] : overloads_) {
                if (!info.isPublic) names.push_back(name);
            }
            return names;
        }();
    }

    // ==================== TYPE MANAGEMENT ====================
    template <typename U = TypeT>
    typename std::enable_if<!std::is_void<U>::value, bool>::type
    addType(const std::string& name, U type, bool isPublic = true) {
        if (threadSafeMode_) {
            return withWriteLock([&]() {
                types_[name] = std::move(type);
                return true;
            });
        } else {
            types_[name] = std::move(type);
            return true;
        }
    }

    template <typename U = TypeT>
    typename std::enable_if<!std::is_void<U>::value, U>::type
    getType(const std::string& name) const {
        if (threadSafeMode_) {
            return withReadLock([&]() -> U {
                if (auto it = types_.find(name); it != types_.end()) {
                    return it->second;
                }
                auto parentPtr = parent_.lock();
                return parentPtr ? parentPtr->template getType<U>(name) : U{};
            });
        } else {
            if (auto it = types_.find(name); it != types_.end()) {
                return it->second;
            }
            auto parentPtr = parent_.lock();
            return parentPtr ? parentPtr->template getType<U>(name) : U{};
        }
    }

    template <typename U = TypeT>
    typename std::enable_if<!std::is_void<U>::value, bool>::type
    typeExists(const std::string& name) const {
        if (threadSafeMode_) {
            return withReadLock([&]() {
                if (types_.count(name)) return true;
                auto parentPtr = parent_.lock();
                return parentPtr && parentPtr->template typeExists<U>(name);
            });
        } else {
            if (types_.count(name)) return true;
            auto parentPtr = parent_.lock();
            return parentPtr && parentPtr->template typeExists<U>(name);
        }
    }
    
    template <typename U = TypeT>
    typename std::enable_if<!std::is_void<U>::value, bool>::type
    removeType(const std::string& name) {
        if (threadSafeMode_) {
            return withWriteLock([&]() {
                return types_.erase(name) > 0;
            });
        } else {
            return types_.erase(name) > 0;
        }
    }
    
    template <typename U = TypeT>
    typename std::enable_if<!std::is_void<U>::value, std::vector<std::string>>::type
    getAllTypeNames(bool includeParent = true) const {
        if (threadSafeMode_) {
            return withReadLock([&]() {
                std::vector<std::string> names;
                for (const auto& [name, _] : types_) {
                    names.push_back(name);
                }
                if (includeParent) {
                    auto parentPtr = parent_.lock();
                    if (parentPtr) {
                        auto parentNames = parentPtr->template getAllTypeNames<U>(true);
                        names.insert(names.end(), parentNames.begin(), parentNames.end());
                    }
                }
                return names;
            });
        } else {
            std::vector<std::string> names;
            for (const auto& [name, _] : types_) {
                names.push_back(name);
            }
            if (includeParent) {
                auto parentPtr = parent_.lock();
                if (parentPtr) {
                    auto parentNames = parentPtr->template getAllTypeNames<U>(true);
                    names.insert(names.end(), parentNames.begin(), parentNames.end());
                }
            }
            return names;
        }
    }

    // ==================== SCOPE MANAGEMENT ====================
    SymbolTablePtr createChildScope(const std::string& name = "") {
        return threadSafeMode_ ? withWriteLock([&]() {
            auto child = std::make_shared<SymbolTable<T, TypeT>>(getSharedThis(), name);
            children_.push_back(child);
            notifyScopeChanged("create_child");
            return child;
        }) : [&]() {
            auto child = std::make_shared<SymbolTable<T, TypeT>>(getSharedThis(), name);
            children_.push_back(child);
            notifyScopeChanged("create_child");
            return child;
        }();
    }

    SymbolTablePtr getParent() const {
        return parent_.lock();
    }

    SymbolTablePtr getRoot() const {
        return threadSafeMode_ ? withReadLock([&]() {
            auto current = getSharedThis();
            while (auto parent = current->getParent()) {
                current = parent;
            }
            return current;
        }) : [&]() {
            auto current = getSharedThis();
            while (auto parent = current->getParent()) {
                current = parent;
            }
            return current;
        }();
    }
    
    SymbolTablePtr findScope(const std::string& name) const {
        return threadSafeMode_ ? withReadLock([&]() {
            if (name_ == name) return getSharedThis();
            for (const auto& child : children_) {
                if (auto scope = child->findScope(name)) {
                    return scope;
                }
            }
            return SymbolTablePtr{};
        }) : [&]() {
            if (name_ == name) return getSharedThis();
            for (const auto& child : children_) {
                if (auto scope = child->findScope(name)) {
                    return scope;
                }
            }
            return SymbolTablePtr{};
        }();
    }

    std::vector<SymbolTablePtr> getChildScopes() const {
        return threadSafeMode_ ? withReadLock([&]() { return children_; }) : children_;
    }

    std::vector<std::string> getScopePath() const {
        return threadSafeMode_ ? withReadLock([&]() {
            std::vector<std::string> path;
            auto current = getSharedThis();
            while (current) {
                if (!current->name_.empty()) {
                    path.push_back(current->name_);
                }
                current = current->getParent();
            }
            std::reverse(path.begin(), path.end());
            return path;
        }) : [&]() {
            std::vector<std::string> path;
            auto current = getSharedThis();
            while (current) {
                if (!current->name_.empty()) {
                    path.push_back(current->name_);
                }
                current = current->getParent();
            }
            std::reverse(path.begin(), path.end());
            return path;
        }();
    }

    size_t getScopeDepth() const {
        return threadSafeMode_ ? withReadLock([&]() {
            size_t depth = 0;
            auto current = getSharedThis();
            while (auto parent = current->getParent()) {
                ++depth;
                current = parent;
            }
            return depth;
        }) : [&]() {
            size_t depth = 0;
            auto current = getSharedThis();
            while (auto parent = current->getParent()) {
                ++depth;
                current = parent;
            }
            return depth;
        }();
    }
    
    std::string getName() const {
        return threadSafeMode_ ? withReadLock([&]() { return name_; }) : name_;
    }

    void setName(const std::string& name) {
        if (threadSafeMode_) {
            withWriteLock([&]() { name_ = name; });
        } else {
            name_ = name;
        }
    }

    std::string getFullPath() const {
        return threadSafeMode_ ? withReadLock([&]() { return generateScopePath(); }) : generateScopePath();
    }

    bool isGlobalScope() const {
        return threadSafeMode_ ? withReadLock([&]() { return !parent_.lock(); }) : !parent_.lock();
    }

    bool isChildOf(const SymbolTablePtr& potentialParent) const {
        return threadSafeMode_ ? withReadLock([&]() {
            auto current = getSharedThis();
            while (current) {
                if (current == potentialParent) return true;
                current = current->getParent();
            }
            return false;
        }) : [&]() {
            auto current = getSharedThis();
            while (current) {
                if (current == potentialParent) return true;
                current = current->getParent();
            }
            return false;
        }();
    }
    
    // ==================== MODULE MANAGEMENT ====================
    static bool defineModule(const std::string& path, SymbolTablePtr module) {
        std::lock_guard<std::mutex> lock(modulesMutex_);
        if (!SymbolTableUtils::isValidModuleName(path)) {
            return false;
        }
        globalModules_[path] = ModuleInfoType{path, path, "", module};
        return true;
    }

    static SymbolTablePtr getModuleByPath(const std::string& path) {
        std::lock_guard<std::mutex> lock(modulesMutex_);
        auto it = globalModules_.find(path);
        return it != globalModules_.end() ? it->second.symbolTable : nullptr;
    }

    static bool moduleExists(const std::string& path) {
        std::lock_guard<std::mutex> lock(modulesMutex_);
        return globalModules_.count(path) > 0;
    }

    static bool unloadModule(const std::string& path) {
        std::lock_guard<std::mutex> lock(modulesMutex_);
        return globalModules_.erase(path) > 0;
    }

    static std::vector<std::string> getAllModulePaths() {
        std::lock_guard<std::mutex> lock(modulesMutex_);
        std::vector<std::string> paths;
        for (const auto& [path, _] : globalModules_) {
            paths.push_back(path);
        }
        return paths;
    }

    static void clearAllModules() {
        std::lock_guard<std::mutex> lock(modulesMutex_);
        globalModules_.clear();
    }
    
    static bool addModuleDependency(const std::string& modulePath, const std::string& dependencyPath) {
        std::lock_guard<std::mutex> lock(modulesMutex_);
        auto it = globalModules_.find(modulePath);
        if (it == globalModules_.end()) return false;
        it->second.dependencies.insert(dependencyPath);
        return true;
    }

    static std::vector<std::string> getModuleDependencies(const std::string& modulePath) {
        std::lock_guard<std::mutex> lock(modulesMutex_);
        auto it = globalModules_.find(modulePath);
        if (it == globalModules_.end()) return {};
        return std::vector<std::string>(it->second.dependencies.begin(), it->second.dependencies.end());
    }

    static bool hasCircularDependency(const std::string& modulePath) {
        std::lock_guard<std::mutex> lock(modulesMutex_);
        std::unordered_set<std::string> visited, recursionStack;
        return checkCircularDependency(modulePath, visited, recursionStack);
    }
    
    bool aliasModule(const std::string& alias, const std::string& fullPath) {
        return threadSafeMode_ ? withWriteLock([&]() {
            localModuleAliases_[alias] = fullPath;
            return true;
        }) : [&]() {
            localModuleAliases_[alias] = fullPath;
            return true;
        }();
    }

    bool removeModuleAlias(const std::string& alias) {
        return threadSafeMode_ ? withWriteLock([&]() {
            return localModuleAliases_.erase(alias) > 0;
        }) : localModuleAliases_.erase(alias) > 0;
    }

    SymbolTablePtr getModule(const std::string& alias) const {
        return threadSafeMode_ ? withReadLock([&]() {
            auto it = localModuleAliases_.find(alias);
            if (it != localModuleAliases_.end()) {
                return getModuleByPath(it->second);
            }
            return SymbolTablePtr{};
        }) : [&]() {
            auto it = localModuleAliases_.find(alias);
            if (it != localModuleAliases_.end()) {
                return getModuleByPath(it->second);
            }
            return SymbolTablePtr{};
        }();
    }

    std::string resolveModuleAlias(const std::string& alias) const {
        return threadSafeMode_ ? withReadLock([&]() {
            auto it = localModuleAliases_.find(alias);
            return it != localModuleAliases_.end() ? it->second : "";
        }) : [&]() {
            auto it = localModuleAliases_.find(alias);
            return it != localModuleAliases_.end() ? it->second : "";
        }();
    }

    std::vector<std::string> getModuleAliases() const {
        return threadSafeMode_ ? withReadLock([&]() {
            std::vector<std::string> aliases;
            for (const auto& [alias, _] : localModuleAliases_) {
                aliases.push_back(alias);
            }
            return aliases;
        }) : [&]() {
            std::vector<std::string> aliases;
            for (const auto& [alias, _] : localModuleAliases_) {
                aliases.push_back(alias);
            }
            return aliases;
        }();
    }
    
    bool importSymbol(const std::string& symbolName, const std::string& modulePath, 
                     const std::string& alias = "") {
        return threadSafeMode_ ? withWriteLock([&]() {
            auto module = getModuleByPath(modulePath);
            if (!module) return false;
            auto result = module->lookup(symbolName);
            if (!result.found() || !result.isPublic) return false;
            auto targetName = alias.empty() ? symbolName : alias;
            if (result.isVariable() || result.isConstantValue()) {
                return setSymbolInternal(targetName, result.value, result.isConstant, true);
            } else if (result.isOverloadSet()) {
                for (const auto& overload : result.overloads) {
                    addOverloadable(targetName, overload, true);
                }
                return true;
            }
            return false;
        }) : [&]() {
            auto module = getModuleByPath(modulePath);
            if (!module) return false;
            auto result = module->lookup(symbolName);
            if (!result.found() || !result.isPublic) return false;
            auto targetName = alias.empty() ? symbolName : alias;
            if (result.isVariable() || result.isConstantValue()) {
                return setSymbolInternal(targetName, result.value, result.isConstant, true);
            } else if (result.isOverloadSet()) {
                for (const auto& overload : result.overloads) {
                    addOverloadable(targetName, overload, true);
                }
                return true;
            }
            return false;
        }();
    }

    bool importAllPublicSymbols(const std::string& modulePath, const std::string& prefix = "") {
        return threadSafeMode_ ? withWriteLock([&]() {
            auto module = getModuleByPath(modulePath);
            if (!module) return false;
            for (const auto& name : module->getPublicSymbolNames()) {
                auto result = module->lookup(name);
                if (!result.found()) continue;
                auto targetName = prefix.empty() ? name : prefix + "." + name;
                if (result.isVariable() || result.isConstantValue()) {
                    setSymbolInternal(targetName, result.value, result.isConstant, true);
                } else if (result.isOverloadSet()) {
                    for (const auto& overload : result.overloads) {
                        addOverloadable(targetName, overload, true);
                    }
                }
            }
            return true;
        }) : [&]() {
            auto module = getModuleByPath(modulePath);
            if (!module) return false;
            for (const auto& name : module->getPublicSymbolNames()) {
                auto result = module->lookup(name);
                if (!result.found()) continue;
                auto targetName = prefix.empty() ? name : prefix + "." + name;
                if (result.isVariable() || result.isConstantValue()) {
                    setSymbolInternal(targetName, result.value, result.isConstant, true);
                } else if (result.isOverloadSet()) {
                    for (const auto& overload : result.overloads) {
                        addOverloadable(targetName, overload, true);
                    }
                }
            }
            return true;
        }();
    }

    bool exportSymbol(const std::string& symbolName) {
        return threadSafeMode_ ? withWriteLock([&]() {
            auto result = lookup(symbolName);
            if (result.found()) {
                exportedSymbols_.insert(symbolName);
                return true;
            }
            return false;
        }) : [&]() {
            auto result = lookup(symbolName);
            if (result.found()) {
                exportedSymbols_.insert(symbolName);
                return true;
            }
            return false;
        }();
    }

    bool exportAllSymbols() {
        return threadSafeMode_ ? withWriteLock([&]() {
            for (const auto& name : getPublicSymbolNames()) {
                exportedSymbols_.insert(name);
            }
            return true;
        }) : [&]() {
            for (const auto& name : getPublicSymbolNames()) {
                exportedSymbols_.insert(name);
            }
            return true;
        }();
    }

    std::vector<std::string> getExportedSymbols() const {
        return threadSafeMode_ ? withReadLock([&]() {
            return std::vector<std::string>(exportedSymbols_.begin(), exportedSymbols_.end());
        }) : std::vector<std::string>(exportedSymbols_.begin(), exportedSymbols_.end());
    }
    
    // ==================== ADVANCED FEATURES ====================
    void setValidationFunction(ValidationFunction validator) {
        if (threadSafeMode_) {
            withWriteLock([&]() { validator = std::move(validator); });
        } else {
            validator = std::move(validator);
        }
    }

    bool validateSymbol(const std::string& name, const T& value) const {
        return threadSafeMode_ ? withReadLock([&]() {
            return validator ? validator(name, value) : true;
        }) : validator ? validator(name, value) : true;
    }
    
    void addSymbolChangeListener(const std::string& symbolName, SymbolCallback callback) {
        if (threadSafeMode_) {
            withWriteLock([&]() {
                symbolListeners_[symbolName].push_back(std::move(callback));
            });
        } else {
            symbolListeners_[symbolName].push_back(std::move(callback));
        }
    }

    void removeSymbolChangeListener(const std::string& symbolName) {
        if (threadSafeMode_) {
            withWriteLock([&]() { symbolListeners_.erase(symbolName); });
        } else {
            symbolListeners_.erase(symbolName);
        }
    }

    void addScopeChangeListener(ScopeCallback callback) {
        if (threadSafeMode_) {
            withWriteLock([&]() { scopeListeners_.push_back(std::move(callback)); });
        } else {
            scopeListeners_.push_back(std::move(callback));
        }
    }

    void removeScopeChangeListener(ScopeCallback callback) {
        if (threadSafeMode_) {
            withWriteLock([&]() {
                scopeListeners_.erase(
                    std::remove(scopeListeners_.begin(), scopeListeners_.end(), callback),
                    scopeListeners_.end());
            });
        } else {
            scopeListeners_.erase(
                std::remove(scopeListeners_.begin(), scopeListeners_.end(), callback),
                scopeListeners_.end());
        }
    }
    
    bool setMultipleVariables(const std::unordered_map<std::string, T>& variables, bool isPublic = true) {
        return threadSafeMode_ ? withWriteLock([&]() {
            bool success = true;
            for (const auto& [name, value] : variables) {
                success &= setSymbolInternal(name, value, false, isPublic);
            }
            return success;
        }) : [&]() {
            bool success = true;
            for (const auto& [name, value] : variables) {
                success &= setSymbolInternal(name, value, false, isPublic);
            }
            return success;
        }();
    }

    bool setMultipleConstants(const std::unordered_map<std::string, T>& constants, bool isPublic = true) {
        return threadSafeMode_ ? withWriteLock([&]() {
            bool success = true;
            for (const auto& [name, value] : constants) {
                success &= setSymbolInternal(name, value, true, isPublic);
            }
            return success;
        }) : [&]() {
            bool success = true;
            for (const auto& [name, value] : constants) {
                success &= setSymbolInternal(name, value, true, isPublic);
            }
            return success;
        }();
    }

    std::unordered_map<std::string, T> getMultipleValues(const std::vector<std::string>& names) const {
        return threadSafeMode_ ? withReadLock([&]() {
            std::unordered_map<std::string, T> result;
            for (const auto& name : names) {
                try {
                    result[name] = getValue(name);
                } catch (...) {}
            }
            return result;
        }) : [&]() {
            std::unordered_map<std::string, T> result;
            for (const auto& name : names) {
                try {
                    result[name] = getValue(name);
                } catch (...) {}
            }
            return result;
        }();
    }
    
    std::string serialize(bool includePrivate = false, int maxDepth = -1) const {
        return threadSafeMode_ ? withReadLock([&]() {
            return serializeSymbols(includePrivate, 0, maxDepth);
        }) : serializeSymbols(includePrivate, 0, maxDepth);
    }

    std::unordered_map<std::string, std::string> getDebugInfo() const {
        return threadSafeMode_ ? withReadLock([&]() {
            std::unordered_map<std::string, std::string> info;
            info["name"] = name_;
            info["symbol_count"] = std::to_string(symbols_.size());
            info["overload_count"] = std::to_string(overloads_.size());
            info["type_count"] = std::to_string(types_.size());
            info["child_scope_count"] = std::to_string(children_.size());
            info["module_alias_count"] = std::to_string(localModuleAliases_.size());
            return info;
        }) : [&]() {
            std::unordered_map<std::string, std::string> info;
            info["name"] = name_;
            info["symbol_count"] = std::to_string(symbols_.size());
            info["overload_count"] = std::to_string(overloads_.size());
            info["type_count"] = std::to_string(types_.size());
            info["child_scope_count"] = std::to_string(children_.size());
            info["module_alias_count"] = std::to_string(localModuleAliases_.size());
            return info;
        }();
    }
    
    void optimize() {
        if (threadSafeMode_) {
            withWriteLock([&]() {
                symbols_.rehash(symbols_.size());
                overloads_.rehash(overloads_.size());
                types_.rehash(types_.size());
                localModuleAliases_.rehash(localModuleAliases_.size());
                exportedSymbols_.rehash(exportedSymbols_.size());
            });
        } else {
            symbols_.rehash(symbols_.size());
            overloads_.rehash(overloads_.size());
            types_.rehash(types_.size());
            localModuleAliases_.rehash(localModuleAliases_.size());
            exportedSymbols_.rehash(exportedSymbols_.size());
        }
    }

    void clearUnusedSymbols() {
        if (threadSafeMode_) {
            withWriteLock([&]() {
                for (auto it = symbols_.begin(); it != symbols_.end();) {
                    if (!it->second.isConstant && exportedSymbols_.count(it->first) == 0) {
                        it = symbols_.erase(it);
                    } else {
                        ++it;
                    }
                }
            });
        } else {
            for (auto it = symbols_.begin(); it != symbols_.end();) {
                if (!it->second.isConstant && exportedSymbols_.count(it->first) == 0) {
                    it = symbols_.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    size_t getMemoryFootprint() const {
        return threadSafeMode_ ? withReadLock([&]() {
            size_t size = sizeof(*this);
            size += symbols_.size() * sizeof(decltype(symbols_)::value_type);
            size += overloads_.size() * sizeof(decltype(overloads_)::value_type);
            size += types_.size() * sizeof(decltype(types_)::value_type);
            size += children_.size() * sizeof(SymbolTablePtr);
            size += localModuleAliases_.size() * sizeof(decltype(localModuleAliases_)::value_type);
            size += exportedSymbols_.size() * sizeof(std::string);
            return size;
        }) : [&]() {
            size_t size = sizeof(*this);
            size += symbols_.size() * sizeof(decltype(symbols_)::value_type);
            size += overloads_.size() * sizeof(decltype(overloads_)::value_type);
            size += types_.size() * sizeof(decltype(types_)::value_type);
            size += children_.size() * sizeof(SymbolTablePtr);
            size += localModuleAliases_.size() * sizeof(decltype(localModuleAliases_)::value_type);
            size += exportedSymbols_.size() * sizeof(std::string);
            return size;
        }();
    }

    void reserve(size_t expectedSymbols) {
        if (threadSafeMode_) {
            withWriteLock([&]() {
                symbols_.reserve(expectedSymbols);
                overloads_.reserve(expectedSymbols);
                types_.reserve(expectedSymbols);
            });
        } else {
            symbols_.reserve(expectedSymbols);
            overloads_.reserve(expectedSymbols);
            types_.reserve(expectedSymbols);
        }
    }
    
    void enableThreadSafety(bool enable = true) {
        if (threadSafeMode_ != enable) {
            withWriteLock([&]() { threadSafeMode_ = enable; });
        }
    }

    bool isThreadSafe() const {
        return threadSafeMode_;
    }
    
    struct Statistics {
        size_t totalSymbols = 0;
        size_t variableCount = 0;
        size_t constantCount = 0;
        size_t overloadCount = 0;
        size_t typeCount = 0;
        size_t childScopeCount = 0;
        size_t moduleAliasCount = 0;
        size_t memoryUsage = 0;
    };
    
    Statistics getStatistics() const {
        return threadSafeMode_ ? withReadLock([&]() {
            Statistics stats;
            stats.totalSymbols = symbols_.size() + overloads_.size();
            for (const auto& [_, info] : symbols_) {
                if (info.isConstant) ++stats.constantCount;
                else ++stats.variableCount;
            }
            stats.overloadCount = overloads_.size();
            stats.typeCount = types_.size();
            stats.childScopeCount = children_.size();
            stats.moduleAliasCount = localModuleAliases_.size();
            stats.memoryUsage = getMemoryFootprint();
            return stats;
        }) : [&]() {
            Statistics stats;
            stats.totalSymbols = symbols_.size() + overloads_.size();
            for (const auto& [_, info] : symbols_) {
                if (info.isConstant) ++stats.constantCount;
                else ++stats.variableCount;
            }
            stats.overloadCount = overloads_.size();
            stats.typeCount = types_.size();
            stats.childScopeCount = children_.size();
            stats.moduleAliasCount = localModuleAliases_.size();
            stats.memoryUsage = getMemoryFootprint();
            return stats;
        }();
    }
    
    // ==================== ITERATOR SUPPORT ====================
    class SymbolIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<std::string, T>;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;
        
        SymbolIterator(const SymbolTable* table, bool atEnd = false)
            : table_(table), atEnd_(atEnd) {
            if (!atEnd && table && !table->symbols_.empty()) {
                current_ = table->symbols_.begin();
                currentPair_ = {current_->first, current_->second.value};
            }
        }
        
        reference operator*() const {
            return currentPair_;
        }

        pointer operator->() const {
            return &currentPair_;
        }

        SymbolIterator& operator++() {
            if (!atEnd_ && table_) {
                if (++current_ == table_->symbols_.end()) {
                    atEnd_ = true;
                } else {
                    currentPair_ = {current_->first, current_->second.value};
                }
            }
            return *this;
        }

        SymbolIterator operator++(int) {
            SymbolIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const SymbolIterator& other) const {
            return table_ == other.table_ && atEnd_ == other.atEnd_ && 
                   (atEnd_ || current_ == other.current_);
        }

        bool operator!=(const SymbolIterator& other) const {
            return !(*this == other);
        }
        
    private:
        const SymbolTable* table_;
        typename std::unordered_map<std::string, SymbolInfoType>::const_iterator current_;
        bool atEnd_;
        mutable value_type currentPair_;
    };
    
    SymbolIterator begin() const {
        return SymbolIterator(this);
    }

    SymbolIterator end() const {
        return SymbolIterator(this, true);
    }
    
    class PublicSymbolRange {
    public:
        explicit PublicSymbolRange(const SymbolTable& table) : table_(table) {}
        SymbolIterator begin() const { return table_.begin(); }
        SymbolIterator end() const { return table_.end(); }
    private:
        const SymbolTable& table_;
    };
    
    PublicSymbolRange publicSymbols() const {
        return PublicSymbolRange(*this);
    }

private:
    // ==================== PRIVATE MEMBERS ====================
    static inline std::unordered_map<std::string, ModuleInfoType> globalModules_;
    static inline std::mutex modulesMutex_;
    std::string name_;
    WeakSymbolTablePtr parent_;
    std::vector<SymbolTablePtr> children_;
    
    std::unordered_map<std::string, SymbolInfoType> symbols_;
    std::unordered_map<std::string, OverloadInfoType> overloads_;
    
    typename std::conditional<
        std::is_void<TypeT>::value, 
        int, 
        std::unordered_map<std::string, TypeT>
    >::type types_;
    
    std::unordered_map<std::string, std::string> localModuleAliases_;
    std::unordered_set<std::string> exportedSymbols_;
    
    ValidationFunction validator;
    std::unordered_map<std::string, std::vector<SymbolCallback>> symbolListeners_;
    std::vector<ScopeCallback> scopeListeners_;
    
    mutable std::shared_mutex mutex_;
    bool threadSafeMode_;
    
    // ==================== PRIVATE METHODS ====================
    bool setSymbolInternal(const std::string& name, T value, bool isConstant, bool isPublic) {
        if (validator && !validator(name, value)) {
            return false;
        }
        symbols_[name] = SymbolInfoType{std::move(value), name, "", isConstant, isPublic};
        notifySymbolChanged(name, value);
        return true;
    }

    bool removeSymbolInternal(const std::string& name) {
        bool removed = symbols_.erase(name) > 0 || overloads_.erase(name) > 0;
        if (removed) notifySymbolChanged(name, T{});
        return removed;
    }

    LookupResultType lookupInternal(const std::string& name, bool searchParent = true) const {
        LookupResultType result;
        if (auto it = symbols_.find(name); it != symbols_.end()) {
            result.type = it->second.isConstant ? LookupResultType::Type::Constant : LookupResultType::Type::Variable;
            result.value = it->second.value;
            result.name = name;
            result.isConstant = it->second.isConstant;
            result.isPublic = it->second.isPublic;
            return result;
        }
        if (auto it = overloads_.find(name); it != overloads_.end()) {
            result.type = LookupResultType::Type::Overload;
            result.overloads = it->second.overloads;
            result.name = name;
            result.isPublic = it->second.isPublic;
            return result;
        }
        if (searchParent) {
            auto parentPtr = parent_.lock();
            if (parentPtr) {
                return parentPtr->lookupInternal(name, true);
            }
        }
        return result;
    }
    
    void notifySymbolChanged(const std::string& name, const T& value) {
        if (auto it = symbolListeners_.find(name); it != symbolListeners_.end()) {
            for (const auto& callback : it->second) {
                callback(name, value);
            }
        }
    }

    void notifyScopeChanged(const std::string& operation) {
        for (const auto& callback : scopeListeners_) {
            callback(operation);
        }
    }
    
    static std::vector<std::string> resolveDependencyOrder(const std::string& modulePath) {
        std::vector<std::string> order;
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> recursionStack;
        if (!checkCircularDependency(modulePath, visited, recursionStack)) {
            order.push_back(modulePath);
            auto deps = getModuleDependencies(modulePath);
            for (const auto& dep : deps) {
                auto depOrder = resolveDependencyOrder(dep);
                order.insert(order.end(), depOrder.begin(), depOrder.end());
            }
        }
        return order;
    }

    static bool checkCircularDependency(const std::string& modulePath, 
                                       std::unordered_set<std::string>& visited,
                                       std::unordered_set<std::string>& recursionStack) {
        if (recursionStack.count(modulePath)) return true;
        if (visited.count(modulePath)) return false;
        
        visited.insert(modulePath);
        recursionStack.insert(modulePath);
        
        auto it = globalModules_.find(modulePath);
        if (it != globalModules_.end()) {
            for (const auto& dep : it->second.dependencies) {
                if (checkCircularDependency(dep, visited, recursionStack)) {
                    return true;
                }
            }
        }
        
        recursionStack.erase(modulePath);
        return false;
    }
    
    SymbolTablePtr getSharedThis() {
        return this->shared_from_this();
    }

    const SymbolTablePtr getSharedThis() const {
        return this->shared_from_this();
    }

    std::string generateScopePath() const {
        auto path = getScopePath();
        return SymbolTableUtils::joinPath(path);
    }

    void updateChildrenParentReferences() {
        if (threadSafeMode_) {
            withWriteLock([&]() {
                for (auto& child : children_) {
                    child->parent_ = getSharedThis();
                }
            });
        } else {
            for (auto& child : children_) {
                child->parent_ = getSharedThis();
            }
        }
    }
    
    template<typename Func>
    auto withReadLock(Func&& func) const -> decltype(func()) {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return func();
    }
    
    template<typename Func>
    auto withWriteLock(Func&& func) -> decltype(func()) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        return func();
    }
    
    std::string serializeSymbols(bool includePrivate, int currentDepth, int maxDepth) const {
        std::stringstream ss;
        if (maxDepth >= 0 && currentDepth > maxDepth) return "";
        
        ss << "Scope: " << name_ << "\n";
        for (const auto& [name, info] : symbols_) {
            if (!includePrivate && !info.isPublic) continue;
            ss << "  Symbol: " << name << (info.isConstant ? " [const]" : "") << "\n";
        }
        for (const auto& [name, info] : overloads_) {
            if (!includePrivate && !info.isPublic) continue;
            ss << "  Overloads: " << name << " (" << info.overloads.size() << ")\n";
        }
        for (const auto& child : children_) {
            ss << child->serializeSymbols(includePrivate, currentDepth + 1, maxDepth);
        }
        return ss.str();
    }

    std::string serializeTypes(bool includePrivate) const {
        std::stringstream ss;
        for (const auto& [name, _] : types_) {
            ss << "  Type: " << name << "\n";
        }
        return ss.str();
    }
};

namespace SymbolTableUtils {
    template<typename T>
    struct is_symbol_table : std::false_type {};
    
    template<typename T, typename TypeT>
    struct is_symbol_table<SymbolTable<T, TypeT>> : std::true_type {};
    
    template<typename T>
    constexpr bool is_symbol_table_v = is_symbol_table<T>::value;
}

} // namespace Omniscript