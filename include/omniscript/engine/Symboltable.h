#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <omniscript/Core.h>
#include <omniscript/Core/Types.h>
#include <omniscript/Core/Expression.h>
#include <omniscript/debuggingtools/console.h>
#include <omniscript/omniscript_pch.h>

template <typename T>
class SymbolTable : public std::enable_shared_from_this<SymbolTable<T>> {
public:
    SymbolTable(std::shared_ptr<SymbolTable<T>> parent = nullptr, const std::string& name = "") 
        : parent_(std::move(parent)), name_(name) {}

    // ==================== TYPE MANAGEMENT ====================
    void addType(const std::string& name, T type) {
        types_[name] = std::move(type);
    }

    T getType(const std::string& name) const {
        if (auto it = types_.find(name); it != types_.end()) return it->second;
        return parent_ ? parent_->getType(name) : nullptr;
    }

    bool typeExists(const std::string& name) const {
        return types_.count(name) || (parent_ && parent_->typeExists(name));
    }

    // ==================== VALUE STORAGE ====================
    void set(const std::string& name, T value) {
        setVariable(name, value);
    }

    void setVariable(const std::string& name, T value) {
        if (overloadables_.count(name)) {
            console.error("Cannot define variable '" + name + "' because an overloaded function already exists with that name.");
            return;
        }
        if (auto it = constants_.find(name); it != constants_.end()) {
            console.error("Cannot define variable '" + name + "' because a constant already exists with that name.");
            return;
        }
        variables_[name] = std::move(value);
    }

    void setConstant(const std::string& name, T value) {
        if (overloadables_.count(name)) {
            console.error("Cannot define variable '" + name + "' because an overloaded function already exists with that name.");
            return;
        }
        if (auto it = variables_.find(name); it != variables_.end()) {
            console.error("Cannot define constant '" + name + "' because a variable already exists with that name.");
            return;
        }
        if (auto it = constants_.find(name); it != constants_.end()) {
            console.error("Constant '" + name + "' already exists in scope '" + name + "' and cannot be reassigned");
            return;
        } 
        constants_[name] = std::move(value);
    }

    void addOverloadable(const std::string& name, T value) {
        overloadables_[name].push_back(std::move(value));
    }

    T get(const std::string& name) const {
        return getValue(name);
    }

    T getValue(const std::string& name) const {
        if (auto it = variables_.find(name); it != variables_.end()) return it->second;
        if (auto it = constants_.find(name); it != constants_.end()) return it->second;
        DEBUG_LOG("Symbol  '" + name + "' was not found in scope '" + name_ + "'.");
        auto result = parent_ ? parent_->getValue(name) : nullptr;
        
        if (!result) {
            console.error("Symbol '" + name + "' was not found in scope '" + name_ + "'.");
        }

        return result;
    }

    std::vector<T> getOverloads(const std::string& name) const {
        if (auto it = overloadables_.find(name); it != overloadables_.end()) {
            return it->second;
        }
        DEBUG_LOG("Symbol '" + name + "' was not found in scope '" + name_ + "'.");
        return parent_ ? parent_->getOverloads(name) : std::vector<T>{};
    }    

    T* getPointerToValue(const std::string& name) {
        if (auto it = variables_.find(name); it != variables_.end()) return &it->second;
        if (auto it = constants_.find(name); it != constants_.end()) return &it->second;
        return parent_ ? parent_->getPointerToValue(name) : nullptr;
    }
    

    // ==================== SCOPE MANAGEMENT ====================
    std::shared_ptr<SymbolTable<T>> createChildScope(const std::string& name) {
        auto child = std::make_shared<SymbolTable<T>>(this->shared_from_this(), name);
        return child;
    }

    std::shared_ptr<SymbolTable<T>> getParent() const { return parent_; }
    
    std::string getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

private:
    std::string name_;
    std::shared_ptr<SymbolTable<T>> parent_;
    
    // Type definitions (structs, enums, aliases)
    std::unordered_map<std::string, T> types_;
    
    // Variable storage
    std::unordered_map<std::string, T> variables_;
    std::unordered_map<std::string, std::vector<T>> overloadables_;
    std::unordered_map<std::string, T> constants_;
};

#endif
