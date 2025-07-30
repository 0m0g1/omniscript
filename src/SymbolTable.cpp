#include <stdexcept>
#include <omniscript/omniscript_pch.h>
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

template <typename T, typename TypeT>
SymbolTable<T, TypeT>::SymbolTable(SymbolTablePtr parent, const std::string& name)
    : name_(name), parent_(parent), threadSafeMode_(false) {}

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
    return lookup(name).found();
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::setVariable(const std::string& name, T value, bool isPublic) {
    return threadSafeMode_ ? withWriteLock([&]() {
        return setSymbolInternal(name, std::move(value), false, isPublic);
    }) : setSymbolInternal(name, std::move(value), false, isPublic);
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::setConstant(const std::string& name, T value, bool isPublic) {
    return threadSafeMode_ ? withWriteLock([&]() {
        return setSymbolInternal(name, std::move(value), true, isPublic);
    }) : setSymbolInternal(name, std::move(value), true, isPublic);
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::addOverloadable(const std::string& name, T value, bool isPublic) {
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

template <typename T, typename TypeT>
T SymbolTable<T, TypeT>::getValue(const std::string& name) const {
    auto result = lookup(name);
    if (result.found()) {
        return result.value;
    }
    throw std::runtime_error("Symbol not found: " + name);
}

template <typename T, typename TypeT>
std::vector<T> SymbolTable<T, TypeT>::getOverloads(const std::string& name) const {
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

template <typename T, typename TypeT>
std::shared_ptr<SymbolTable<T, TypeT>> SymbolTable<T, TypeT>::createChildScope(const std::string& name) {
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

template <typename T, typename TypeT>
std::shared_ptr<SymbolTable<T, TypeT>> SymbolTable<T, TypeT>::getParent() const {
    return parent_.lock();
}

template <typename T, typename TypeT>
std::string SymbolTable<T, TypeT>::getName() const {
    return threadSafeMode_ ? withReadLock([&]() { return name_; }) : name_;
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::setName(const std::string& name) {
    if (threadSafeMode_) {
        withWriteLock([&]() { name_ = name; });
    } else {
        name_ = name;
    }
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::defineModule(const std::string& path, SymbolTablePtr module) {
    std::lock_guard<std::mutex> lock(modulesMutex_);
    if (!SymbolTableUtils::isValidModuleName(path)) {
        return false;
    }
    globalModules_[path] = ModuleInfoType{path, path, "", module};
    return true;
}

template <typename T, typename TypeT>
template <typename Func>
auto SymbolTable<T, TypeT>::withReadLock(Func&& func) const -> decltype(func()) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return func();
}

template <typename T, typename TypeT>
template <typename Func>
auto SymbolTable<T, TypeT>::withWriteLock(Func&& func) -> decltype(func()) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return func();
}

template <typename T, typename TypeT>
bool SymbolTable<T, TypeT>::setSymbolInternal(const std::string& name, T value, bool isConstant, bool isPublic) {
    if (validator_ && !validator_(name, value)) {
        return false;
    }
    symbols_[name] = SymbolInfoType{std::move(value), name, "", isConstant, isPublic};
    notifySymbolChanged(name, value);
    return true;
}

template <typename T, typename TypeT>
typename SymbolTable<T, TypeT>::LookupResultType SymbolTable<T, TypeT>::lookupInternal(const std::string& name, bool searchParent) const {
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

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::notifySymbolChanged(const std::string& name, const T& value) {
    if (auto it = symbolListeners_.find(name); it != symbolListeners_.end()) {
        for (const auto& callback : it->second) {
            callback(name, value);
        }
    }
}

template <typename T, typename TypeT>
void SymbolTable<T, TypeT>::notifyScopeChanged(const std::string& operation) {
    for (const auto& callback : scopeListeners_) {
        callback(operation);
    }
}

template <typename T, typename TypeT>
std::shared_ptr<SymbolTable<T, TypeT>> SymbolTable<T, TypeT>::getSharedThis() {
    return this->shared_from_this();
}

template <typename T, typename TypeT>
const std::shared_ptr<SymbolTable<T, TypeT>> SymbolTable<T, TypeT>::getSharedThis() const {
    return this->shared_from_this();
}

} // namespace Omniscript