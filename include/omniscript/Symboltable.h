#pragma once

#include <omniscript/omniscript_pch.h>

template <typename T, typename TypeT = void>
class SymbolTable : public std::enable_shared_from_this<SymbolTable<T, TypeT>> {
public:
    SymbolTable(std::shared_ptr<SymbolTable<T, TypeT>> parent = nullptr, const std::string& name = "")
        : parent_(parent), name_(name) {}

    // ==================== VALUE MANAGEMENT ====================
    void set(const std::string& name, T value) { setVariable(name, value); }

    void setVariable(const std::string& name, T value) {
        if (overloadables_.count(name)) {
            DEBUG_LOG("Cannot define variable '" + name + "' because an overloaded function exists.");
            return;
        }
        if (constants_.count(name)) {
            DEBUG_LOG("Cannot define variable '" + name + "' because a constant exists.");
            return;
        }
        variables_[name] = value;
    }

    void setConstant(const std::string& name, T value) {
        if (overloadables_.count(name)) {
            DEBUG_LOG("Cannot define constant '" + name + "' because an overloaded function exists.");
            return;
        }
        if (variables_.count(name)) {
            DEBUG_LOG("Cannot define constant '" + name + "' because a variable exists.");
            return;
        }
        if (constants_.count(name)) {
            DEBUG_LOG("Constant '" + name + "' already exists.");
            return;
        }
        constants_[name] = value;
    }

    void addOverloadable(const std::string& name, T value) {
        overloadables_[name].push_back(value);
    }

    T get(const std::string& name) const { return getValue(name); }

    T getValue(const std::string& name) const {
        if (auto it = variables_.find(name); it != variables_.end()) return it->second;
        if (auto it = constants_.find(name); it != constants_.end()) return it->second;
        return parent_ ? parent_->getValue(name) : nullptr;
    }

    T* getPointerToValue(const std::string& name) {
        if (auto it = variables_.find(name); it != variables_.end()) return &it->second;
        if (auto it = constants_.find(name); it != constants_.end()) return &it->second;
        return parent_ ? parent_->getPointerToValue(name) : nullptr;
    }

    std::vector<T> getOverloads(const std::string& name) const {
        if (auto it = overloadables_.find(name); it != overloadables_.end()) return it->second;
        return parent_ ? parent_->getOverloads(name) : std::vector<T>{};
    }

    bool exists(const std::string& name) const {
        return variables_.count(name) ||
               constants_.count(name) ||
               overloadables_.count(name) ||
               localModuleAliases_.count(name) ||
               (parent_ && parent_->exists(name));
    }

    // ==================== TYPE MANAGEMENT ====================
    template <typename U = TypeT>
    typename std::enable_if<!std::is_void<U>::value, void>::type
    addType(const std::string& name, U type) {
        types_[name] = type;
    }

    template <typename U = TypeT>
    typename std::enable_if<!std::is_void<U>::value, U>::type
    getType(const std::string& name) const {
        if (auto it = types_.find(name); it != types_.end()) return it->second;
        return parent_ ? parent_->template getType<U>(name) : nullptr;
    }

    template <typename U = TypeT>
    typename std::enable_if<!std::is_void<U>::value, bool>::type
    typeExists(const std::string& name) const {
        return types_.count(name) || (parent_ && parent_->template typeExists<U>(name));
    }

    // ==================== SCOPE / MODULE MANAGEMENT ====================
    std::shared_ptr<SymbolTable<T, TypeT>> createChildScope(const std::string& name = "") {
        return std::make_shared<SymbolTable<T, TypeT>>(this->shared_from_this(), name);
    }

    std::shared_ptr<SymbolTable<T, TypeT>> getParent() const { return parent_; }

    std::string getName() const {
        if (name_.empty()) {
            if (parent_) {
                return parent_->getName();
            }
            return "";
        }
        return name_; 
    }
    void setName(const std::string& name) { name_ = name; }

    // ----- Module Registry (Global) -----
    static void defineModule(const std::string& path, std::shared_ptr<SymbolTable<T, TypeT>> module) {
        globalModules_[path] = module;
    }

    static std::shared_ptr<SymbolTable<T, TypeT>> getModuleByPath(const std::string& path) {
        auto it = globalModules_.find(path);
        return it != globalModules_.end() ? it->second : nullptr;
    }

    // ----- Local aliasing -----
    void aliasModule(const std::string& alias, const std::string& fullPath) {
        if (exists(alias)) {
            DEBUG_LOG("Cannot alias '" + alias + "' because a symbol already exists with that name.");
            return;
        }
        if (!globalModules_.count(fullPath)) {
            DEBUG_LOG("Cannot alias '" + alias + "' because module path '" + fullPath + "' not found.");
            return;
        }
        localModuleAliases_[alias] = fullPath;
    }

    std::shared_ptr<SymbolTable<T, TypeT>> getModule(const std::string& alias) const {
        if (auto it = localModuleAliases_.find(alias); it != localModuleAliases_.end()) {
            auto globalIt = globalModules_.find(it->second);
            if (globalIt != globalModules_.end()) return globalIt->second;
        }
        return parent_ ? parent_->getModule(alias) : nullptr;
    }

private:
    std::string name_;
    std::shared_ptr<SymbolTable<T, TypeT>> parent_;

    std::unordered_map<std::string, T> variables_;
    std::unordered_map<std::string, T> constants_;
    std::unordered_map<std::string, std::vector<T>> overloadables_;

    typename std::conditional<std::is_void<TypeT>::value, int, std::unordered_map<std::string, TypeT>>::type types_;

    // ====== Module support ======
    static inline std::unordered_map<std::string, std::shared_ptr<SymbolTable<T, TypeT>>> globalModules_;
    std::unordered_map<std::string, std::string> localModuleAliases_; // alias → full path
};
