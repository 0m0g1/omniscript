#include <omniscript/SymbolTable.h>
#include <omniscript/utils.h>
#include <omniscript/Console.h>
#include <algorithm>
#include <sstream>
#include <queue>
#include <regex>
#include <iomanip>

namespace Omniscript {

using console = Omniscript::Console;

// Static member initialization
template <typename T, typename TypeT>
std::unordered_map<std::string, ModuleInfo<T, TypeT>> SymbolTable<T, TypeT>::globalModules_;

template <typename T, typename TypeT>
std::mutex SymbolTable<T, TypeT>::modulesMutex_;

// ==================== CONSTRUCTOR AND BASIC SETUP ====================
template <typename T, typename TypeT>
SymbolTable<T, TypeT>::SymbolTable(SymbolTablePtr parent, const std::string& name)
    : name_(name)
    , parent_(parent)
    , threadSafeMode_(false)
{
    if (parent) {
        parent->children_.push_back(getSharedThis());
    }
    
    // Initialize with reasonable capacity
    symbols_.reserve(64);
    overloads_.reserve(16);
    
    if constexpr (!std::is_void_v<TypeT>) {
        types_.reserve(32);
    }
    
    DEBUG_LOG("Created SymbolTable: " + name_);
}

// ==================== BASIC VALUE MANAGEMENT ====================
template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::set(const std::string& name, T value, bool isPublic) {
    setVariable(name, std::move(value), isPublic);
}

template <typename T, typename TypeT>
T SymbolTable<T, TypeT>::get(const std::string& name) const {
    return getValue(name);
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::exists(const std::string& name) const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            return symbols_.count(name) || 
                   overloads_.count(name) || 
                   localModuleAliases_.count(name) ||
                   (parent_.lock() && parent_.lock()->exists(name));
        });
    } else {
        return symbols_.count(name) || 
               overloads_.count(name) || 
               localModuleAliases_.count(name) ||
               (parent_.lock() && parent_.lock()->exists(name));
    }
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::setVariable(const std::string& name, T value, bool isPublic) {
    return setSymbolInternal(name, std::move(value), false, isPublic);
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::setConstant(const std::string& name, T value, bool isPublic) {
    return setSymbolInternal(name, std::move(value), true, isPublic);
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::addOverloadable(const std::string& name, T value, bool isPublic) {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            if (symbols_.count(name)) {
                console.debug("Cannot add overload '" + name + "' because a symbol already exists.");
                return false;
            }
            
            if (!overloads_.count(name)) {
                overloads_[name] = OverloadInfoType(name, isPublic);
            }
            
            overloads_[name].addOverload(std::move(value));
            notifySymbolChanged(name, value);
            return true;
        });
    } else {
        if (symbols_.count(name)) {
            console.debug("Cannot add overload '" + name + "' because a symbol already exists.");
            return false;
        }
        
        if (!overloads_.count(name)) {
            overloads_[name] = OverloadInfoType(name, isPublic);
        }
        
        overloads_[name].addOverload(std::move(value));
        notifySymbolChanged(name, value);
        return true;
    }
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::removeSymbol(const std::string& name) {
    return removeSymbolInternal(name);
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::updateVariable(const std::string& name, T newValue) {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            auto it = symbols_.find(name);
            if (it == symbols_.end()) {
                return false;
            }
            if (it->second.isConstant) {
                console.debug("Cannot update constant '" + name + "'.");
                return false;
            }
            if (validator_ && !validator_(name, newValue)) {
                console.debug("Validation failed for symbol '" + name + "'.");
                return false;
            }
            it->second.value = std::move(newValue);
            notifySymbolChanged(name, it->second.value);
            return true;
        });
    } else {
        auto it = symbols_.find(name);
        if (it == symbols_.end()) {
            return false;
        }
        if (it->second.isConstant) {
            console.debug("Cannot update constant '" + name + "'.");
            return false;
        }
        if (validator_ && !validator_(name, newValue)) {
            console.debug("Validation failed for symbol '" + name + "'.");
            return false;
        }
        it->second.value = std::move(newValue);
        notifySymbolChanged(name, it->second.value);
        return true;
    }
}

// ==================== ADVANCED GETTERS ====================
template <typename T, typename TypeT>
T SymbolTable<T, TypeT>::getValue(const std::string& name) const {
    auto result = lookup(name);
    if (result.found()) {
        if (result.isVariable() || result.isConstantValue()) {
            return result.value;
        }
    }
    console.debug("Symbol '" + name + "' not found, returning default value.");
    return T{};
}

template <typename T, typename TypeT>
T* SymbolTable<T, TypeT>::getPointerToValue(const std::string& name) {
    if (threadSafeMode_) {
        console.debug("Warning: getPointerToValue is not thread-safe in thread-safe mode.");
    }
    
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
        return &it->second.value;
    }
    
    console.debug("Symbol '" + name + "' not found for pointer access.");
    return nullptr;
}

template <typename T, typename TypeT>
const T* SymbolTable<T, TypeT>::getConstPointerToValue(const std::string& name) const {
    if (threadSafeMode_) {
        return withReadLock([&]() -> const T* {
            auto it = symbols_.find(name);
            return it != symbols_.end() ? &it->second.value : nullptr;
        });
    } else {
        auto it = symbols_.find(name);
        return it != symbols_.end() ? &it->second.value : nullptr;
    }
}

template <typename T, typename TypeT>
std::vector<T> SymbolTable<T, TypeT>::getOverloads(const std::string& name) const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            auto it = overloads_.find(name);
            if (it != overloads_.end()) {
                return it->second.overloads;
            }
            auto parentPtr = parent_.lock();
            return parentPtr ? parentPtr->getOverloads(name) : std::vector<T>{};
        });
    } else {
        auto it = overloads_.find(name);
        if (it != overloads_.end()) {
            return it->second.overloads;
        }
        auto parentPtr = parent_.lock();
        return parentPtr ? parentPtr->getOverloads(name) : std::vector<T>{};
    }
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::LookupResultType 
SymbolTable<T, TypeT>::lookup(const std::string& name) const {
    return lookupInternal(name, true);
}

template <typename T, typename TypeT>
std::optional<typename SymbolTable<T, TypeT>::SymbolInfoType> 
SymbolTable<T, TypeT>::getSymbolInfo(const std::string& name) const {
    if (threadSafeMode_) {
        return withReadLock([&]() -> std::optional<SymbolInfoType> {
            auto it = symbols_.find(name);
            return it != symbols_.end() ? std::optional<SymbolInfoType>{it->second} : std::nullopt;
        });
    } else {
        auto it = symbols_.find(name);
        return it != symbols_.end() ? std::optional<SymbolInfoType>{it->second} : std::nullopt;
    }
}

template <typename T, typename TypeT>
std::vector<std::string> SymbolTable<T, TypeT>::getAllSymbolNames(bool includeParent) const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            std::vector<std::string> names;
            names.reserve(symbols_.size() + overloads_.size() + localModuleAliases_.size());
            
            for (const auto& [name, info] : symbols_) {
                names.push_back(name);
            }
            for (const auto& [name, info] : overloads_) {
                names.push_back(name);
            }
            for (const auto& [alias, path] : localModuleAliases_) {
                names.push_back(alias);
            }
            
            if (includeParent) {
                auto parentPtr = parent_.lock();
                if (parentPtr) {
                    auto parentNames = parentPtr->getAllSymbolNames(true);
                    names.insert(names.end(), parentNames.begin(), parentNames.end());
                }
            }
            
            std::sort(names.begin(), names.end());
            names.erase(std::unique(names.begin(), names.end()), names.end());
            
            return names;
        });
    } else {
        std::vector<std::string> names;
        names.reserve(symbols_.size() + overloads_.size() + localModuleAliases_.size());
        
        for (const auto& [name, info] : symbols_) {
            names.push_back(name);
        }
        for (const auto& [name, info] : overloads_) {
            names.push_back(name);
        }
        for (const auto& [alias, path] : localModuleAliases_) {
            names.push_back(alias);
        }
        
        if (includeParent) {
            auto parentPtr = parent_.lock();
            if (parentPtr) {
                auto parentNames = parentPtr->getAllSymbolNames(true);
                names.insert(names.end(), parentNames.begin(), parentNames.end());
            }
        }
        
        std::sort(names.begin(), names.end());
        names.erase(std::unique(names.begin(), names.end()), names.end());
        
        return names;
    }
}

template <typename T, typename TypeT>
std::vector<std::string> SymbolTable<T, TypeT>::getPublicSymbolNames() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            std::vector<std::string> names;
            names.reserve(symbols_.size() + overloads_.size());
            
            for (const auto& [name, info] : symbols_) {
                if (info.isPublic) {
                    names.push_back(name);
                }
            }
            for (const auto& [name, info] : overloads_) {
                if (info.isPublic) {
                    names.push_back(name);
                }
            }
            
            std::sort(names.begin(), names.end());
            return names;
        });
    } else {
        std::vector<std::string> names;
        names.reserve(symbols_.size() + overloads_.size());
        
        for (const auto& [name, info] : symbols_) {
            if (info.isPublic) {
                names.push_back(name);
            }
        }
        for (const auto& [name, info] : overloads_) {
            if (info.isPublic) {
                names.push_back(name);
            }
        }
        
        std::sort(names.begin(), names.end());
        return names;
    }
}

template <typename T, typename TypeT>
std::vector<std::string> SymbolTable<T, TypeT>::getPrivateSymbolNames() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            std::vector<std::string> names;
            names.reserve(symbols_.size() + overloads_.size());
            
            for (const auto& [name, info] : symbols_) {
                if (!info.isPublic) {
                    names.push_back(name);
                }
            }
            for (const auto& [name, info] : overloads_) {
                if (!info.isPublic) {
                    names.push_back(name);
                }
            }
            
            std::sort(names.begin(), names.end());
            return names;
        });
    } else {
        std::vector<std::string> names;
        names.reserve(symbols_.size() + overloads_.size());
        
        for (const auto& [name, info] : symbols_) {
            if (!info.isPublic) {
                names.push_back(name);
            }
        }
        for (const auto& [name, info] : overloads_) {
            if (!info.isPublic) {
                names.push_back(name);
            }
        }
        
        std::sort(names.begin(), names.end());
        return names;
    }
}

// ==================== TYPE MANAGEMENT ====================
template <typename T, typename TypeT>
template <typename U>
typename std::enable_if<!std::is_void<U>::value, bool>::type
SymbolTable<T, TypeT>::addType(const std::string& name, U type, bool isPublic) {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            types_[name] = std::move(type);
            console.debug("Added type '" + name + "' to scope: " + getFullPath());
            return true;
        });
    } else {
        types_[name] = std::move(type);
        console.debug("Added type '" + name + "' to scope: " + getFullPath());
        return true;
    }
}

template <typename T, typename TypeT>
template <typename U>
typename std::enable_if<!std::is_void<U>::value, U>::type
SymbolTable<T, TypeT>::getType(const std::string& name) const {
    if (threadSafeMode_) {
        return withReadLock([&]() -> U {
            auto it = types_.find(name);
            if (it != types_.end()) {
                return it->second;
            }
            auto parentPtr = parent_.lock();
            if (parentPtr) {
                return parentPtr->getType<U>(name);
            }
            console.debug("Type '" + name + "' not found in scope: " + getFullPath());
            return U{};
        });
    } else {
        auto it = types_.find(name);
        if (it != types_.end()) {
            return it->second;
        }
        auto parentPtr = parent_.lock();
        if (parentPtr) {
            return parentPtr->getType<U>(name);
        }
        console.debug("Type '" + name + "' not found in scope: " + getFullPath());
        return U{};
    }
}

template <typename T, typename TypeT>
template <typename U>
typename std::enable_if<!std::is_void<U>::value, bool>::type
SymbolTable<T, TypeT>::typeExists(const std::string& name) const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            if (types_.count(name)) {
                return true;
            }
            auto parentPtr = parent_.lock();
            return parentPtr && parentPtr->typeExists<U>(name);
        });
    } else {
        if (types_.count(name)) {
            return true;
        }
        auto parentPtr = parent_.lock();
        return parentPtr && parentPtr->typeExists<U>(name);
    }
}

template <typename T, typename TypeT>
template <typename U>
typename std::enable_if<!std::is_void<U>::value, bool>::type
SymbolTable<T, TypeT>::removeType(const std::string& name) {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            auto it = types_.find(name);
            if (it != types_.end()) {
                types_.erase(it);
                console.debug("Removed type '" + name + "' from scope: " + getFullPath());
                return true;
            }
            console.debug("Type '" + name + "' not found for removal in scope: " + getFullPath());
            return false;
        });
    } else {
        auto it = types_.find(name);
        if (it != types_.end()) {
            types_.erase(it);
            console.debug("Removed type '" + name + "' from scope: " + getFullPath());
            return true;
        }
        console.debug("Type '" + name + "' not found for removal in scope: " + getFullPath());
        return false;
    }
}

template <typename T, typename TypeT>
template <typename U>
typename std::enable_if<!std::is_void<U>::value, std::vector<std::string>>::type
SymbolTable<T, TypeT>::getAllTypeNames(bool includeParent) const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            std::vector<std::string> names;
            names.reserve(types_.size());
            
            for (const auto& [name, type] : types_) {
                names.push_back(name);
            }
            
            if (includeParent) {
                auto parentPtr = parent_.lock();
                if (parentPtr) {
                    auto parentNames = parentPtr->getAllTypeNames<U>(true);
                    names.insert(names.end(), parentNames.begin(), parentNames.end());
                }
            }
            
            std::sort(names.begin(), names.end());
            names.erase(std::unique(names.begin(), names.end()), names.end());
            
            return names;
        });
    } else {
        std::vector<std::string> names;
        names.reserve(types_.size());
        
        for (const auto& [name, type] : types_) {
            names.push_back(name);
        }
        
        if (includeParent) {
            auto parentPtr = parent_.lock();
            if (parentPtr) {
                auto parentNames = parentPtr->getAllTypeNames<U>(true);
                names.insert(names.end(), parentNames.begin(), parentNames.end());
            }
        }
        
        std::sort(names.begin(), names.end());
        names.erase(std::unique(names.begin(), names.end()), names.end());
        
        return names;
    }
}

// ==================== SCOPE MANAGEMENT ====================
template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolTablePtr 
SymbolTable<T, TypeT>::createChildScope(const std::string& name) {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            auto child = std::make_shared<SymbolTable<T, TypeT>>(getSharedThis(), name);
            children_.push_back(child);
            notifyScopeChanged("create_child:" + name);
            console.debug("Created child scope: " + child->getFullPath());
            return child;
        });
    } else {
        auto child = std::make_shared<SymbolTable<T, TypeT>>(getSharedThis(), name);
        children_.push_back(child);
        notifyScopeChanged("create_child:" + name);
        console.debug("Created child scope: " + child->getFullPath());
        return child;
    }
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolTablePtr 
SymbolTable<T, TypeT>::getParent() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            return parent_.lock();
        });
    } else {
        return parent_.lock();
    }
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolTablePtr 
SymbolTable<T, TypeT>::getRoot() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            auto current = getSharedThis();
            while (auto parent = current->getParent()) {
                current = parent;
            }
            return current;
        });
    } else {
        auto current = getSharedThis();
        while (auto parent = current->getParent()) {
            current = parent;
        }
        return current;
    }
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolTablePtr 
SymbolTable<T, TypeT>::findScope(const std::string& name) const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            if (name_ == name) {
                return getSharedThis();
            }
            for (const auto& child : children_) {
                if (child->getName() == name) {
                    return child;
                }
                auto found = child->findScope(name);
                if (found) {
                    return found;
                }
            }
            return SymbolTablePtr();
        });
    } else {
        if (name_ == name) {
            return getSharedThis();
        }
        for (const auto& child : children_) {
            if (child->getName() == name) {
                return child;
            }
            auto found = child->findScope(name);
            if (found) {
                return found;
            }
        }
        return SymbolTablePtr();
    }
}

template <typename T, typename TypeT>
std::vector<typename SymbolTable<T, TypeT>::SymbolTablePtr> 
SymbolTable<T, TypeT>::getChildScopes() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            return children_;
        });
    } else {
        return children_;
    }
}

template <typename T, typename TypeT>
std::vector<std::string> SymbolTable<T, TypeT>::getScopePath() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            return splitPath(generateScopePath());
        });
    } else {
        return splitPath(generateScopePath());
    }
}

template <typename T, typename TypeT>
size_t SymbolTable<T, TypeT>::getScopeDepth() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            size_t depth = 0;
            auto current = getSharedThis();
            while (auto parent = current->getParent()) {
                depth++;
                current = parent;
            }
            return depth;
        });
    } else {
        size_t depth = 0;
        auto current = getSharedThis();
        while (auto parent = current->getParent()) {
            depth++;
            current = parent;
        }
        return depth;
    }
}

template <typename T, typename TypeT>
std::string SymbolTable<T, TypeT>::getName() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            return name_;
        });
    } else {
        return name_;
    }
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::setName(const std::string& name) {
    if (threadSafeMode_) {
        withWriteLock([&]() {
            name_ = name;
            notifyScopeChanged("rename:" + name);
            console.debug("Renamed scope to: " + name_);
        });
    } else {
        name_ = name;
        notifyScopeChanged("rename:" + name);
        console.debug("Renamed scope to: " + name_);
    }
}

template <typename T, typename TypeT>
std::string SymbolTable<T, TypeT>::getFullPath() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            return generateScopePath();
        });
    } else {
        return generateScopePath();
    }
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::isGlobalScope() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            return !parent_.lock();
        });
    } else {
        return !parent_.lock();
    }
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::isChildOf(const SymbolTablePtr& potentialParent) const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            auto current = getSharedThis();
            while (auto parent = current->getParent()) {
                if (parent == potentialParent) {
                    return true;
                }
                current = parent;
            }
            return false;
        });
    } else {
        auto current = getSharedThis();
        while (auto parent = current->getParent()) {
            if (parent == potentialParent) {
                return true;
            }
            current = parent;
        }
        return false;
    }
}

// ==================== MODULE MANAGEMENT ====================
template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::defineModule(const std::string& path, SymbolTablePtr module) {
    std::lock_guard<std::mutex> lock(modulesMutex_);
    if (!SymbolTableUtils::isValidModuleName(path)) {
        console.error("Invalid module path: " + path);
        return false;
    }
    std::string normalizedPath = SymbolTableUtils::normalizePath(path);
    if (globalModules_.count(normalizedPath)) {
        console.debug("Module already defined: " + normalizedPath);
        return false;
    }
    globalModules_[normalizedPath] = ModuleInfoType(normalizedPath, normalizedPath, module);
    console.debug("Defined module: " + normalizedPath);
    return true;
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolTablePtr 
SymbolTable<T, TypeT>::getModuleByPath(const std::string& path) {
    std::lock_guard<std::mutex> lock(modulesMutex_);
    std::string normalizedPath = SymbolTableUtils::normalizePath(path);
    auto it = globalModules_.find(normalizedPath);
    if (it != globalModules_.end()) {
        return it->second.symbolTable;
    }
    console.debug("Module not found: " + normalizedPath);
    return nullptr;
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::moduleExists(const std::string& path) {
    std::lock_guard<std::mutex> lock(modulesMutex_);
    std::string normalizedPath = SymbolTableUtils::normalizePath(path);
    return globalModules_.count(normalizedPath) > 0;
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::unloadModule(const std::string& path) {
    std::lock_guard<std::mutex> lock(modulesMutex_);
    std::string normalizedPath = SymbolTableUtils::normalizePath(path);
    if (globalModules_.count(normalizedPath)) {
        if (globalModules_[normalizedPath].isSystem) {
            console.debug("Cannot unload system module: " + normalizedPath);
            return false;
        }
        globalModules_.erase(normalizedPath);
        console.debug("Unloaded module: " + normalizedPath);
        return true;
    }
    console.debug("Module not found for unloading: " + normalizedPath);
    return false;
}

template <typename T, typename TypeT>
std::vector<std::string> SymbolTable<T, TypeT>::getAllModulePaths() {
    std::lock_guard<std::mutex> lock(modulesMutex_);
    std::vector<std::string> paths;
    paths.reserve(globalModules_.size());
    for (const auto& [path, info] : globalModules_) {
        paths.push_back(path);
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::clearAllModules() {
    std::lock_guard<std::mutex> lock(modulesMutex_);
    auto it = globalModules_.begin();
    while (it != globalModules_.end()) {
        if (!it->second.isSystem) {
            it = globalModules_.erase(it);
        } else {
            ++it;
        }
    }
    console.debug("Cleared all non-system modules.");
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::addModuleDependency(const std::string& modulePath, const std::string& dependencyPath) {
    std::lock_guard<std::mutex> lock(modulesMutex_);
    std::string normalizedModule = SymbolTableUtils::normalizePath(modulePath);
    std::string normalizedDep = SymbolTableUtils::normalizePath(dependencyPath);
    
    if (!globalModules_.count(normalizedModule) || !globalModules_.count(normalizedDep)) {
        console.error("Module or dependency not found: " + normalizedModule + ", " + normalizedDep);
        return false;
    }
    
    if (hasCircularDependency(normalizedModule)) {
        console.error("Adding dependency would create a circular dependency: " + normalizedDep);
        return false;
    }
    
    globalModules_[normalizedModule].dependencies.insert(normalizedDep);
    console.debug("Added dependency " + normalizedDep + " to module " + normalizedModule);
    return true;
}

template <typename T, typename TypeT>
std::vector<std::string> SymbolTable<T, TypeT>::getModuleDependencies(const std::string& modulePath) {
    std::lock_guard<std::mutex> lock(modulesMutex_);
    std::string normalizedPath = SymbolTableUtils::normalizePath(modulePath);
    auto it = globalModules_.find(normalizedPath);
    if (it != globalModules_.end()) {
        return std::vector<std::string>(it->second.dependencies.begin(), it->second.dependencies.end());
    }
    return {};
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::hasCircularDependency(const std::string& modulePath) {
    std::lock_guard<std::mutex> lock(modulesMutex_);
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursionStack;
    return checkCircularDependency(SymbolTableUtils::normalizePath(modulePath), visited, recursionStack);
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::aliasModule(const std::string& alias, const std::string& fullPath) {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            std::string normalizedPath = SymbolTableUtils::normalizePath(fullPath);
            if (!moduleExists(normalizedPath)) {
                console.error("Cannot alias non-existent module: " + normalizedPath);
                return false;
            }
            if (symbols_.count(alias) || overloads_.count(alias)) {
                console.error("Alias '" + alias + "' conflicts with existing symbol.");
                return false;
            }
            localModuleAliases_[alias] = normalizedPath;
            console.debug("Aliased module " + normalizedPath + " as " + alias);
            return true;
        });
    } else {
        std::string normalizedPath = SymbolTableUtils::normalizePath(fullPath);
        if (!moduleExists(normalizedPath)) {
            console.error("Cannot alias non-existent module: " + normalizedPath);
            return false;
        }
        if (symbols_.count(alias) || overloads_.count(alias)) {
            console.error("Alias '" + alias + "' conflicts with existing symbol.");
            return false;
        }
        localModuleAliases_[alias] = normalizedPath;
        console.debug("Aliased module " + normalizedPath + " as " + alias);
        return true;
    }
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::removeModuleAlias(const std::string& alias) {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            auto it = localModuleAliases_.find(alias);
            if (it != localModuleAliases_.end()) {
                localModuleAliases_.erase(it);
                console.debug("Removed module alias: " + alias);
                return true;
            }
            console.debug("Module alias not found: " + alias);
            return false;
        });
    } else {
        auto it = localModuleAliases_.find(alias);
        if (it != localModuleAliases_.end()) {
            localModuleAliases_.erase(it);
            console.debug("Removed module alias: " + alias);
            return true;
        }
        console.debug("Module alias not found: " + alias);
        return false;
    }
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolTablePtr 
SymbolTable<T, TypeT>::getModule(const std::string& alias) const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            auto it = localModuleAliases_.find(alias);
            if (it != localModuleAliases_.end()) {
                return getModuleByPath(it->second);
            }
            auto parentPtr = parent_.lock();
            return parentPtr ? parentPtr->getModule(alias) : nullptr;
        });
    } else {
        auto it = localModuleAliases_.find(alias);
        if (it != localModuleAliases_.end()) {
            return getModuleByPath(it->second);
        }
        auto parentPtr = parent_.lock();
        return parentPtr ? parentPtr->getModule(alias) : nullptr;
    }
}

template <typename T, typename TypeT>
std::string SymbolTable<T, TypeT>::resolveModuleAlias(const std::string& alias) const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            auto it = localModuleAliases_.find(alias);
            if (it != localModuleAliases_.end()) {
                return it->second;
            }
            auto parentPtr = parent_.lock();
            return parentPtr ? parentPtr->resolveModuleAlias(alias) : "";
        });
    } else {
        auto it = localModuleAliases_.find(alias);
        if (it != localModuleAliases_.end()) {
            return it->second;
        }
        auto parentPtr = parent_.lock();
        return parentPtr ? parentPtr->resolveModuleAlias(alias) : "";
    }
}

template <typename T, typename TypeT>
std::vector<std::string> SymbolTable<T, TypeT>::getModuleAliases() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            std::vector<std::string> aliases;
            aliases.reserve(localModuleAliases_.size());
            for (const auto& [alias, path] : localModuleAliases_) {
                aliases.push_back(alias);
            }
            std::sort(aliases.begin(), aliases.end());
            return aliases;
        });
    } else {
        std::vector<std::string> aliases;
        aliases.reserve(localModuleAliases_.size());
        for (const auto& [alias, path] : localModuleAliases_) {
            aliases.push_back(alias);
        }
        std::sort(aliases.begin(), aliases.end());
        return aliases;
    }
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::importSymbol(const std::string& symbolName, const std::string& modulePath, const std::string& alias) {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            auto module = getModuleByPath(modulePath);
            if (!module) {
                console.error("Cannot import from non-existent module: " + modulePath);
                return false;
            }
            auto result = module->lookup(symbolName);
            if (!result.found() || !result.isPublic) {
                console.error("Symbol '" + symbolName + "' not found or not public in module: " + modulePath);
                return false;
            }
            std::string targetName = alias.empty() ? symbolName : alias;
            if (result.isVariable() || result.isConstantValue()) {
                return setSymbolInternal(targetName, result.value, result.isConstantValue(), true);
            }
            if (result.isOverloadSet()) {
                for (const auto& overload : result.overloads) {
                    if (!addOverloadable(targetName, overload, true)) {
                        return false;
                    }
                }
                return true;
            }
            console.error("Cannot import symbol of type: " + std::to_string(static_cast<int>(result.type)));
            return false;
        });
    } else {
        auto module = getModuleByPath(modulePath);
        if (!module) {
            console.error("Cannot import from non-existent module: " + modulePath);
            return false;
        }
        auto result = module->lookup(symbolName);
        if (!result.found() || !result.isPublic) {
            console.error("Symbol '" + symbolName + "' not found or not public in module: " + modulePath);
            return false;
        }
        std::string targetName = alias.empty() ? symbolName : alias;
        if (result.isVariable() || result.isConstantValue()) {
            return setSymbolInternal(targetName, result.value, result.isConstantValue(), true);
        }
        if (result.isOverloadSet()) {
            for (const auto& overload : result.overloads) {
                if (!addOverloadable(targetName, overload, true)) {
                    return false;
                }
            }
            return true;
        }
        console.error("Cannot import symbol of type: " + std::to_string(static_cast<int>(result.type)));
        return false;
    }
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::importAllPublicSymbols(const std::string& modulePath, const std::string& prefix) {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            auto module = getModuleByPath(modulePath);
            if (!module) {
                console.error("Cannot import from non-existent module: " + modulePath);
                return false;
            }
            auto publicSymbols = module->getPublicSymbolNames();
            bool success = true;
            for (const auto& name : publicSymbols) {
                std::string targetName = prefix.empty() ? name : prefix + "." + name;
                if (!importSymbol(name, modulePath, targetName)) {
                    success = false;
                }
            }
            console.debug("Imported " + std::to_string(publicSymbols.size()) + " public symbols from module: " + modulePath);
            return success;
        });
    } else {
        auto module = getModuleByPath(modulePath);
        if (!module) {
            console.error("Cannot import from non-existent module: " + modulePath);
            return false;
        }
        auto publicSymbols = module->getPublicSymbolNames();
        bool success = true;
        for (const auto& name : publicSymbols) {
            std::string targetName = prefix.empty() ? name : prefix + "." + name;
            if (!importSymbol(name, modulePath, targetName)) {
                success = false;
            }
        }
        console.debug("Imported " + std::to_string(publicSymbols.size()) + " public symbols from module: " + modulePath);
        return success;
    }
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::exportSymbol(const std::string& symbolName) {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            if (symbols_.count(symbolName)) {
                symbols_[symbolName].isPublic = true;
                exportedSymbols_.insert(symbolName);
                console.debug("Exported symbol: " + symbolName);
                return true;
            }
            if (overloads_.count(symbolName)) {
                overloads_[symbolName].isPublic = true;
                exportedSymbols_.insert(symbolName);
                console.debug("Exported overload set: " + symbolName);
                return true;
            }
            console.debug("Symbol not found for export: " + symbolName);
            return false;
        });
    } else {
        if (symbols_.count(symbolName)) {
            symbols_[symbolName].isPublic = true;
            exportedSymbols_.insert(symbolName);
            console.debug("Exported symbol: " + symbolName);
            return true;
        }
        if (overloads_.count(symbolName)) {
            overloads_[symbolName].isPublic = true;
            exportedSymbols_.insert(symbolName);
            console.debug("Exported overload set: " + symbolName);
            return true;
        }
        console.debug("Symbol not found for export: " + symbolName);
        return false;
    }
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::exportAllSymbols() {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            bool success = true;
            for (auto& [name, info] : symbols_) {
                info.isPublic = true;
                exportedSymbols_.insert(name);
            }
            for (auto& [name, info] : overloads_) {
                info.isPublic = true;
                exportedSymbols_.insert(name);
            }
            console.debug("Exported all symbols in scope: " + getFullPath());
            return success;
        });
    } else {
        bool success = true;
        for (auto& [name, info] : symbols_) {
            info.isPublic = true;
            exportedSymbols_.insert(name);
        }
        for (auto& [name, info] : overloads_) {
            info.isPublic = true;
            exportedSymbols_.insert(name);
        }
        console.debug("Exported all symbols in scope: " + getFullPath());
        return success;
    }
}

template <typename T, typename TypeT>
std::vector<std::string> SymbolTable<T, TypeT>::getExportedSymbols() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            std::vector<std::string> exported;
            exported.reserve(exportedSymbols_.size());
            for (const auto& name : exportedSymbols_) {
                exported.push_back(name);
            }
            std::sort(exported.begin(), exported.end());
            return exported;
        });
    } else {
        std::vector<std::string> exported;
        exported.reserve(exportedSymbols_.size());
        for (const auto& name : exportedSymbols_) {
            exported.push_back(name);
        }
        std::sort(exported.begin(), exported.end());
        return exported;
    }
}

// ==================== ADVANCED FEATURES ====================
template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::setValidationFunction(ValidationFunction validator) {
    if (threadSafeMode_) {
        withWriteLock([&]() {
            validator_ = std::move(validator);
            console.debug("Set validation function for scope: " + getFullPath());
        });
    } else {
        validator_ = std::move(validator);
        console.debug("Set validation function for scope: " + getFullPath());
    }
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::validateSymbol(const std::string& name, const T& value) const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            return !validator_ || validator_(name, value);
        });
    } else {
        return !validator_ || validator_(name, value);
    }
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::addSymbolChangeListener(const std::string& symbolName, SymbolCallback callback) {
    if (threadSafeMode_) {
        withWriteLock([&]() {
            symbolListeners_[symbolName].push_back(std::move(callback));
            console.debug("Added symbol change listener for: " + symbolName);
        });
    } else {
        symbolListeners_[symbolName].push_back(std::move(callback));
        console.debug("Added symbol change listener for: " + symbolName);
    }
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::removeSymbolChangeListener(const std::string& symbolName) {
    if (threadSafeMode_) {
        withWriteLock([&]() {
            symbolListeners_.erase(symbolName);
            console.debug("Removed symbol change listeners for: " + symbolName);
        });
    } else {
        symbolListeners_.erase(symbolName);
        console.debug("Removed symbol change listeners for: " + symbolName);
    }
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::addScopeChangeListener(ScopeCallback callback) {
    if (threadSafeMode_) {
        withWriteLock([&]() {
            scopeListeners_.push_back(std::move(callback));
            console.debug("Added scope change listener for scope: " + getFullPath());
        });
    } else {
        scopeListeners_.push_back(std::move(callback));
        console.debug("Added scope change listener for scope: " + getFullPath());
    }
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::removeScopeChangeListener(ScopeCallback callback) {
    if (threadSafeMode_) {
        withWriteLock([&]() {
            scopeListeners_.erase(
                std::remove_if(scopeListeners_.begin(), scopeListeners_.end(),
                    [&callback](const auto& cb) { return &cb == &callback; }),
                scopeListeners_.end());
            console.debug("Removed scope change listener for scope: " + getFullPath());
        });
    } else {
        scopeListeners_.erase(
            std::remove_if(scopeListeners_.begin(), scopeListeners_.end(),
                [&callback](const auto& cb) { return &cb == &callback; }),
            scopeListeners_.end());
        console.debug("Removed scope change listener for scope: " + getFullPath());
    }
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::setMultipleVariables(const std::unordered_map<std::string, T>& variables, bool isPublic) {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            bool success = true;
            for (const auto& [name, value] : variables) {
                if (!setSymbolInternal(name, value, false, isPublic)) {
                    success = false;
                }
            }
            console.debug("Set " + std::to_string(variables.size()) + " variables in scope: " + getFullPath());
            return success;
        });
    } else {
        bool success = true;
        for (const auto& [name, value] : variables) {
            if (!setSymbolInternal(name, value, false, isPublic)) {
                success = false;
            }
        }
        console.debug("Set " + std::to_string(variables.size()) + " variables in scope: " + getFullPath());
        return success;
    }
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::setMultipleConstants(const std::unordered_map<std::string, T>& constants, bool isPublic) {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            bool success = true;
            for (const auto& [name, value] : constants) {
                if (!setSymbolInternal(name, value, true, isPublic)) {
                    success = false;
                }
            }
            console.debug("Set " + std::to_string(constants.size()) + " constants in scope: " + getFullPath());
            return success;
        });
    } else {
        bool success = true;
        for (const auto& [name, value] : constants) {
            if (!setSymbolInternal(name, value, true, isPublic)) {
                success = false;
            }
        }
        console.debug("Set " + std::to_string(constants.size()) + " constants in scope: " + getFullPath());
        return success;
    }
}

template <typename T, typename TypeT>
std::unordered_map<std::string, T> SymbolTable<T, TypeT>::getMultipleValues(const std::vector<std::string>& names) const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            std::unordered_map<std::string, T> result;
            for (const auto& name : names) {
                auto value = getValue(name);
                if (value != T{}) {
                    result[name] = value;
                }
            }
            return result;
        });
    } else {
        std::unordered_map<std::string, T> result;
        for (const auto& name : names) {
            auto value = getValue(name);
            if (value != T{}) {
                result[name] = value;
            }
        }
        return result;
    }
}

template <typename T, typename TypeT>
std::string SymbolTable<T, TypeT>::serialize(bool includePrivate, int maxDepth) const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            return serializeSymbols(includePrivate, 0, maxDepth);
        });
    } else {
        return serializeSymbols(includePrivate, 0, maxDepth);
    }
}

template <typename T, typename TypeT>
std::unordered_map<std::string, std::string> SymbolTable<T, TypeT>::getDebugInfo() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            std::unordered_map<std::string, std::string> info;
            info["scope_name"] = name_;
            info["full_path"] = getFullPath();
            info["symbol_count"] = std::to_string(symbols_.size());
            info["overload_count"] = std::to_string(overloads_.size());
            info["child_scope_count"] = std::to_string(children_.size());
            info["module_alias_count"] = std::to_string(localModuleAliases_.size());
            info["exported_symbol_count"] = std::to_string(exportedSymbols_.size());
            if constexpr (!std::is_void_v<TypeT>) {
                info["type_count"] = std::to_string(types_.size());
            }
            return info;
        });
    } else {
        std::unordered_map<std::string, std::string> info;
        info["scope_name"] = name_;
        info["full_path"] = getFullPath();
        info["symbol_count"] = std::to_string(symbols_.size());
        info["overload_count"] = std::to_string(overloads_.size());
        info["child_scope_count"] = std::to_string(children_.size());
        info["module_alias_count"] = std::to_string(localModuleAliases_.size());
        info["exported_symbol_count"] = std::to_string(exportedSymbols_.size());
        if constexpr (!std::is_void_v<TypeT>) {
            info["type_count"] = std::to_string(types_.size());
        }
        return info;
    }
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::optimize() {
    if (threadSafeMode_) {
        withWriteLock([&]() {
            symbols_.shrink_to_fit();
            overloads_.shrink_to_fit();
            localModuleAliases_.shrink_to_fit();
            exportedSymbols_.shrink_to_fit();
            children_.shrink_to_fit();
            if constexpr (!std::is_void_v<TypeT>) {
                types_.shrink_to_fit();
            }
            console.debug("Optimized memory usage for scope: " + getFullPath());
        });
    } else {
        symbols_.shrink_to_fit();
        overloads_.shrink_to_fit();
        localModuleAliases_.shrink_to_fit();
        exportedSymbols_.shrink_to_fit();
        children_.shrink_to_fit();
        if constexpr (!std::is_void_v<TypeT>) {
            types_.shrink_to_fit();
        }
        console.debug("Optimized memory usage for scope: " + getFullPath());
    }
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::clearUnusedSymbols() {
    if (threadSafeMode_) {
        withWriteLock([&]() {
            auto it = symbols_.begin();
            while (it != symbols_.end()) {
                if (!it->second.isConstant && !exportedSymbols_.count(it->first)) {
                    it = symbols_.erase(it);
                } else {
                    ++it;
                }
            }
            console.debug("Cleared unused symbols in scope: " + getFullPath());
        });
    } else {
        auto it = symbols_.begin();
        while (it != symbols_.end()) {
            if (!it->second.isConstant && !exportedSymbols_.count(it->first)) {
                it = symbols_.erase(it);
            } else {
                ++it;
            }
        }
        console.debug("Cleared unused symbols in scope: " + getFullPath());
    }
}

template <typename T, typename TypeT>
size_t SymbolTable<T, TypeT>::getMemoryFootprint() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            size_t memory = sizeof(*this);
            memory += symbols_.size() * (sizeof(std::string) + sizeof(SymbolInfoType));
            memory += overloads_.size() * (sizeof(std::string) + sizeof(OverloadInfoType));
            for (const auto& [name, info] : overloads_) {
                memory += info.overloads.size() * sizeof(T);
            }
            memory += localModuleAliases_.size() * (sizeof(std::string) * 2);
            memory += exportedSymbols_.size() * sizeof(std::string);
            memory += children_.size() * sizeof(SymbolTablePtr);
            if constexpr (!std::is_void_v<TypeT>) {
                memory += types_.size() * (sizeof(std::string) + sizeof(TypeT));
            }
            return memory;
        });
    } else {
        size_t memory = sizeof(*this);
        memory += symbols_.size() * (sizeof(std::string) + sizeof(SymbolInfoType));
        memory += overloads_.size() * (sizeof(std::string) + sizeof(OverloadInfoType));
        for (const auto& [name, info] : overloads_) {
            memory += info.overloads.size() * sizeof(T);
        }
        memory += localModuleAliases_.size() * (sizeof(std::string) * 2);
        memory += exportedSymbols_.size() * sizeof(std::string);
        memory += children_.size() * sizeof(SymbolTablePtr);
        if constexpr (!std::is_void_v<TypeT>) {
            memory += types_.size() * (sizeof(std::string) + sizeof(TypeT));
        }
        return memory;
    }
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::reserve(size_t expectedSymbols) {
    if (threadSafeMode_) {
        withWriteLock([&]() {
            symbols_.reserve(expectedSymbols);
            overloads_.reserve(expectedSymbols / 4);
            localModuleAliases_.reserve(expectedSymbols / 8);
            exportedSymbols_.reserve(expectedSymbols / 2);
            if constexpr (!std::is_void_v<TypeT>) {
                types_.reserve(expectedSymbols / 4);
            }
            console.debug("Reserved space for " + std::to_string(expectedSymbols) + " symbols in scope: " + getFullPath());
        });
    } else {
        symbols_.reserve(expectedSymbols);
        overloads_.reserve(expectedSymbols / 4);
        localModuleAliases_.reserve(expectedSymbols / 8);
        exportedSymbols_.reserve(expectedSymbols / 2);
        if constexpr (!std::is_void_v<TypeT>) {
            types_.reserve(expectedSymbols / 4);
        }
        console.debug("Reserved space for " + std::to_string(expectedSymbols) + " symbols in scope: " + getFullPath());
    }
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::enableThreadSafety(bool enable) {
    threadSafeMode_ = enable;
    console.debug("Thread safety " + std::string(enable ? "enabled" : "disabled") + " for scope: " + getFullPath());
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::isThreadSafe() const {
    return threadSafeMode_;
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::Statistics SymbolTable<T, TypeT>::getStatistics() const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            Statistics stats;
            stats.totalSymbols = symbols_.size() + overloads_.size();
            stats.variableCount = std::count_if(symbols_.begin(), symbols_.end(),
                [](const auto& pair) { return !pair.second.isConstant; });
            stats.constantCount = std::count_if(symbols_.begin(), symbols_.end(),
                [](const auto& pair) { return pair.second.isConstant; });
            stats.overloadCount = overloads_.size();
            stats.childScopeCount = children_.size();
            stats.moduleAliasCount = localModuleAliases_.size();
            stats.memoryUsage = getMemoryFootprint();
            if constexpr (!std::is_void_v<TypeT>) {
                stats.typeCount = types_.size();
            }
            return stats;
        });
    } else {
        Statistics stats;
        stats.totalSymbols = symbols_.size() + overloads_.size();
        stats.variableCount = std::count_if(symbols_.begin(), symbols_.end(),
            [](const auto& pair) { return !pair.second.isConstant; });
        stats.constantCount = std::count_if(symbols_.begin(), symbols_.end(),
            [](const auto& pair) { return pair.second.isConstant; });
        stats.overloadCount = overloads_.size();
        stats.childScopeCount = children_.size();
        stats.moduleAliasCount = localModuleAliases_.size();
        stats.memoryUsage = getMemoryFootprint();
        if constexpr (!std::is_void_v<TypeT>) {
            stats.typeCount = types_.size();
        }
        return stats;
    }
}

// ==================== ITERATOR SUPPORT ====================
template <typename T, typename TypeT>
SymbolTable<T, TypeT>::SymbolIterator::SymbolIterator(const SymbolTable* table, bool atEnd)
    : table_(table), atEnd_(atEnd) {
    if (!atEnd && table) {
        current_ = table_->symbols_.begin();
        if (current_ == table_->symbols_.end()) {
            atEnd_ = true;
        } else {
            currentPair_ = {current_->first, current_->second.value};
        }
    }
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolIterator::reference 
SymbolTable<T, TypeT>::SymbolIterator::operator*() const {
    return currentPair_;
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolIterator::pointer 
SymbolTable<T, TypeT>::SymbolIterator::operator->() const {
    return &currentPair_;
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolIterator& 
SymbolTable<T, TypeT>::SymbolIterator::operator++() {
    if (!atEnd_ && table_) {
        ++current_;
        if (current_ == table_->symbols_.end()) {
            atEnd_ = true;
        } else {
            currentPair_ = {current_->first, current_->second.value};
        }
    }
    return *this;
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolIterator 
SymbolTable<T, TypeT>::SymbolIterator::operator++(int) {
    SymbolIterator tmp = *this;
    ++(*this);
    return tmp;
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::SymbolIterator::operator==(const SymbolIterator& other) const {
    return table_ == other.table_ && atEnd_ == other.atEnd_ && (!atEnd_ ? current_ == other.current_ : true);
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::SymbolIterator::operator!=(const SymbolIterator& other) const {
    return !(*this == other);
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolIterator SymbolTable<T, TypeT>::begin() const {
    return SymbolIterator(this, false);
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolIterator SymbolTable<T, TypeT>::end() const {
    return SymbolIterator(this, true);
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::PublicSymbolRange SymbolTable<T, TypeT>::publicSymbols() const {
    return PublicSymbolRange(*this);
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolIterator 
SymbolTable<T, TypeT>::PublicSymbolRange::begin() const {
    SymbolIterator it(&table_, false);
    while (!it.atEnd_ && !table_.symbols_.at(it.current_->first).isPublic) {
        ++it;
    }
    return it;
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolIterator 
SymbolTable<T, TypeT>::PublicSymbolRange::end() const {
    return SymbolIterator(&table_, true);
}

// ==================== PRIVATE METHODS ====================
template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::setSymbolInternal(const std::string& name, T value, bool isConstant, bool isPublic) {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            if (overloads_.count(name) || localModuleAliases_.count(name)) {
                console.debug("Cannot set symbol '" + name + "' due to conflict with overload or module alias.");
                return false;
            }
            if (validator_ && !validator_(name, value)) {
                console.debug("Validation failed for symbol '" + name + "'.");
                return false;
            }
            SymbolInfoType& info = symbols_[name];
            info.value = std::move(value);
            info.name = name;
            info.isConstant = isConstant;
            info.isPublic = isPublic;
            info.line = 0; // Can be set via external API if needed
            info.column = 0;
            info.filePath = getFullPath();
            notifySymbolChanged(name, info.value);
            console.debug("Set symbol '" + name + "' in scope: " + getFullPath());
            return true;
        });
    } else {
        if (overloads_.count(name) || localModuleAliases_.count(name)) {
            console.debug("Cannot set symbol '" + name + "' due to conflict with overload or module alias.");
            return false;
        }
        if (validator_ && !validator_(name, value)) {
            console.debug("Validation failed for symbol '" + name + "'.");
            return false;
        }
        SymbolInfoType& info = symbols_[name];
        info.value = std::move(value);
        info.name = name;
        info.isConstant = isConstant;
        info.isPublic = isPublic;
        info.line = 0;
        info.column = 0;
        info.filePath = getFullPath();
        notifySymbolChanged(name, info.value);
        console.debug("Set symbol '" + name + "' in scope: " + getFullPath());
        return true;
    }
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::removeSymbolInternal(const std::string& name) {
    if (threadSafeMode_) {
        return withWriteLock([&]() {
            bool removed = false;
            if (symbols_.erase(name)) {
                removed = true;
                console.debug("Removed symbol: " + name);
            }
            if (overloads_.erase(name)) {
                removed = true;
                console.debug("Removed overload set: " + name);
            }
            if (localModuleAliases_.erase(name)) {
                removed = true;
                console.debug("Removed module alias: " + name);
            }
            if (removed) {
                exportedSymbols_.erase(name);
                notifySymbolChanged(name, T{});
            }
            return removed;
        });
    } else {
        bool removed = false;
        if (symbols_.erase(name)) {
            removed = true;
            console.debug("Removed symbol: " + name);
        }
        if (overloads_.erase(name)) {
            removed = true;
            console.debug("Removed overload set: " + name);
        }
        if (localModuleAliases_.erase(name)) {
            removed = true;
            console.debug("Removed module alias: " + name);
        }
        if (removed) {
            exportedSymbols_.erase(name);
            notifySymbolChanged(name, T{});
        }
        return removed;
    }
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::LookupResultType 
SymbolTable<T, TypeT>::lookupInternal(const std::string& name, bool searchParent) const {
    if (threadSafeMode_) {
        return withReadLock([&]() {
            LookupResultType result;
            result.name = name;
            
            if (auto it = symbols_.find(name); it != symbols_.end()) {
                result.type = it->second.isConstant ? LookupResultType::Type::Constant : LookupResultType::Type::Variable;
                result.value = it->second.value;
                result.isConstant = it->second.isConstant;
                result.isPublic = it->second.isPublic;
                result.scope = getSharedThis();
                return result;
            }
            
            if (auto it = overloads_.find(name); it != overloads_.end()) {
                result.type = LookupResultType::Type::Overload;
                result.overloads = it->second.overloads;
                result.isPublic = it->second.isPublic;
                result.scope = getSharedThis();
                return result;
            }
            
            if (auto it = localModuleAliases_.find(name); it != localModuleAliases_.end()) {
                auto module = getModuleByPath(it->second);
                if (module) {
                    result.type = LookupResultType::Type::Module;
                    result.scope = module;
                    return result;
                }
            }
            
            if constexpr (!std::is_void_v<TypeT>) {
                if (types_.count(name)) {
                    result.type = LookupResultType::Type::Type;
                    result.scope = getSharedThis();
                    return result;
                }
            }
            
            if (searchParent) {
                auto parentPtr = parent_.lock();
                if (parentPtr) {
                    return parentPtr->lookupInternal(name, true);
                }
            }
            
            return result;
        });
    } else {
        LookupResultType result;
        result.name = name;
        
        if (auto it = symbols_.find(name); it != symbols_.end()) {
            result.type = it->second.isConstant ? LookupResultType::Type::Constant : LookupResultType::Type::Variable;
            result.value = it->second.value;
            result.isConstant = it->second.isConstant;
            result.isPublic = it->second.isPublic;
            result.scope = getSharedThis();
            return result;
        }
        
        if (auto it = overloads_.find(name); it != overloads_.end()) {
            result.type = LookupResultType::Type::Overload;
            result.overloads = it->second.overloads;
            result.isPublic = it->second.isPublic;
            result.scope = getSharedThis();
            return result;
        }
        
        if (auto it = localModuleAliases_.find(name); it != localModuleAliases_.end()) {
            auto module = getModuleByPath(it->second);
            if (module) {
                result.type = LookupResultType::Type::Module;
                result.scope = module;
                return result;
            }
        }
        
        if constexpr (!std::is_void_v<TypeT>) {
            if (types_.count(name)) {
                result.type = LookupResultType::Type::Type;
                result.scope = getSharedThis();
                return result;
            }
        }
        
        if (searchParent) {
            auto parentPtr = parent_.lock();
            if (parentPtr) {
                return parentPtr->lookupInternal(name, true);
            }
        }
        
        return result;
    }
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::notifySymbolChanged(const std::string& name, const T& value) {
    if (threadSafeMode_) {
        withReadLock([&]() {
            auto it = symbolListeners_.find(name);
            if (it != symbolListeners_.end()) {
                for (const auto& callback : it->second) {
                    callback(name, value);
                }
            }
        });
    } else {
        auto it = symbolListeners_.find(name);
        if (it != symbolListeners_.end()) {
            for (const auto& callback : it->second) {
                callback(name, value);
            }
        }
    }
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::notifyScopeChanged(const std::string& operation) {
    if (threadSafeMode_) {
        withReadLock([&]() {
            for (const auto& callback : scopeListeners_) {
                callback(operation);
            }
        });
    } else {
        for (const auto& callback : scopeListeners_) {
            callback(operation);
        }
    }
}

template <typename T, typename TypeT>
std::vector<std::string> SymbolTable<T, TypeT>::resolveDependencyOrder(const std::string& modulePath) {
    std::lock_guard<std::mutex> lock(modulesMutex_);
    std::vector<std::string> order;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursionStack;
    
    std::function<void(const std::string&)> dfs = [&](const std::string& path) {
        visited.insert(path);
        recursionStack.insert(path);
        
        auto it = globalModules_.find(path);
        if (it != globalModules_.end()) {
            for (const auto& dep : it->second.dependencies) {
                if (!visited.count(dep)) {
                    dfs(dep);
                }
            }
        }
        
        recursionStack.erase(path);
        order.push_back(path);
    };
    
    dfs(SymbolTableUtils::normalizePath(modulePath));
    return order;
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::checkCircularDependency(const std::string& modulePath,
                                                   std::unordered_set<std::string>& visited,
                                                   std::unordered_set<std::string>& recursionStack) {
    std::string normalizedPath = SymbolTableUtils::normalizePath(modulePath);
    visited.insert(normalizedPath);
    recursionStack.insert(normalizedPath);
    
    auto it = globalModules_.find(normalizedPath);
    if (it != globalModules_.end()) {
        for (const auto& dep : it->second.dependencies) {
            if (!visited.count(dep)) {
                if (checkCircularDependency(dep, visited, recursionStack)) {
                    return true;
                }
            } else if (recursionStack.count(dep)) {
                return true;
            }
        }
    }
    
    recursionStack.erase(normalizedPath);
    return false;
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::SymbolTablePtr SymbolTable<T, TypeT>::getSharedThis() {
    return this->shared_from_this();
}

template <typename T, typename TypeT>
const typename SymbolTable<T, TypeT>::SymbolTablePtr SymbolTable<T, TypeT>::getSharedThis() const {
    return this->shared_from_this();
}

template <typename T, typename TypeT>
std::string SymbolTable<T, TypeT>::generateScopePath() const {
    std::vector<std::string> parts;
    auto current = getSharedThis();
    while (current) {
        if (!current->name_.empty()) {
            parts.push_back(current->name_);
        }
        current = current->getParent();
    }
    std::reverse(parts.begin(), parts.end());
    return SymbolTableUtils::joinPath(parts);
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::updateChildrenParentReferences() {
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

template <typename T, typename TypeT>
template<typename Func>
auto SymbolTable<T, TypeT>::withReadLock(Func&& func) const -> decltype(func()) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return func();
}

template <typename T, typename TypeT>
template<typename Func>
auto SymbolTable<T, TypeT>::withWriteLock(Func&& func) -> decltype(func()) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return func();
}

template <typename T, typename TypeT>
std::string SymbolTable<T, TypeT>::serializeSymbols(bool includePrivate, int currentDepth, int maxDepth) const {
    if (maxDepth >= 0 && currentDepth > maxDepth) {
        return "";
    }
    
    std::stringstream ss;
    ss << "Scope: " << getFullPath() << "\n";
    
    for (const auto& [name, info] : symbols_) {
        if (!info.isPublic && !includePrivate) {
            continue;
        }
        ss << "  " << (info.isConstant ? "const " : "var ") 
           << name << " : " << info.type 
           << " = [value] " << (info.isPublic ? "public" : "private") 
           << " @ " << info.filePath << ":" << info.line << ":" << info.column << "\n";
    }
    
    for (const auto& [name, info] : overloads_) {
        if (!info.isPublic && !includePrivate) {
            continue;
        }
        ss << "  overload " << name << " (" << info.getOverloadCount() 
           << " overloads) " << (info.isPublic ? "public" : "private") << "\n";
    }
    
    for (const auto& [alias, path] : localModuleAliases_) {
        ss << "  module alias " << alias << " -> " << path << "\n";
    }
    
    if constexpr (!std::is_void_v<TypeT>) {
        ss << serializeTypes(includePrivate);
    }
    
    for (const auto& child : children_) {
        ss << child->serializeSymbols(includePrivate, currentDepth + 1, maxDepth);
    }
    
    return ss.str();
}

template <typename T, typename TypeT>
std::string SymbolTable<T, TypeT>::serializeTypes(bool includePrivate) const {
    std::stringstream ss;
    for (const auto& [name, type] : types_) {
        ss << "  type " << name << "\n";
    }
    return ss.str();
}

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

// Explicit template instantiation (if needed)
// template class Omniscript::SymbolTable<int>;
// template class Omniscript::SymbolTable<std::string>;