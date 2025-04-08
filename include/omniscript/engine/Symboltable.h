#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <omniscript/Core.h>
#include <omniscript/Core/Value.h>
#include <omniscript/debuggingtools/console.h>
#include <omniscript/omniscript_pch.h>

template <typename T>
class SymbolTable {
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
        variables_[name] = std::move(value);
    }

    void setConstant(const std::string& name, T value) {
        constants_[name] = std::move(value);
    }

    T get(const std::string& name) const {
        return getValue(name);
    }

    T getValue(const std::string& name) const {
        if (auto it = variables_.find(name); it != variables_.end()) return it->second;
        if (auto it = constants_.find(name); it != constants_.end()) return it->second;
        auto result = parent_ ? parent_->getValue(name) : nullptr;

        if (!result) {
            console.error("Symbol '" + name + "' was not found in scope '" + name_ + "'.");
        }

        return result;
    }

    T* getPointerToValue(const std::string& name) {
        if (auto it = variables_.find(name); it != variables_.end()) return &it->second;
        if (auto it = constants_.find(name); it != constants_.end()) return &it->second;
        return parent_ ? parent_->getPointerToValue(name) : nullptr;
    }
    

    // ==================== SCOPE MANAGEMENT ====================
    std::shared_ptr<SymbolTable<T>> createChildScope() {
        return std::make_shared<SymbolTable<T>>(*this);
    }

    std::shared_ptr<SymbolTable<T>> getParent() const { return parent_; }
    
    void setName(const std::string& name) { name_ = name; }

private:
    std::string name_;
    std::shared_ptr<SymbolTable<T>> parent_;
    
    // Type definitions (structs, enums, aliases)
    std::unordered_map<std::string, T> types_;
    
    // Variable storage
    std::unordered_map<std::string, T> variables_;
    std::unordered_map<std::string, T> constants_;
};

#endif
