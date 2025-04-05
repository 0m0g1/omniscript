#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <omniscript/Core.h>
#include <omniscript/Core/Value.h>
#include <omniscript/debuggingtools/console.h>
#include <omniscript/omniscript_pch.h>

class SymbolTable {
public:
    SymbolTable(std::shared_ptr<SymbolTable> parent = nullptr, const std::string& name = "") 
        : parent_(std::move(parent)), name_(name) {}

    // ==================== TYPE MANAGEMENT ====================
    void addType(const std::string& name, std::shared_ptr<Omniscript::Value> type) {
        types_[name] = std::move(type);
    }

    std::shared_ptr<Omniscript::Value> getType(const std::string& name) const {
        if (auto it = types_.find(name); it != types_.end()) return it->second;
        return parent_ ? parent_->getType(name) : nullptr;
    }

    bool typeExists(const std::string& name) const {
        return types_.count(name) || (parent_ && parent_->typeExists(name));
    }

    // ==================== VALUE STORAGE ====================
    void setVariable(const std::string& name, std::shared_ptr<Omniscript::Value> value) {
        variables_[name] = value;
    }

    void setConstant(const std::string& name, std::shared_ptr<Omniscript::Value> value) {
        constants_[name] = value;
    }

    std::shared_ptr<Omniscript::Value> getValue(const std::string& name) const {
        if (auto it = variables_.find(name); it != variables_.end()) return it->second;
        if (auto it = constants_.find(name); it != constants_.end()) return it->second;
        return parent_ ? parent_->getValue(name) : nullptr;
    }

    // ==================== SCOPE MANAGEMENT ====================
    std::shared_ptr<SymbolTable> createChildScope() {
        return std::make_shared<SymbolTable>(*this);
    }

    std::shared_ptr<SymbolTable> getParent() const { return parent_; }
    
    void setName(const std::string& name) { name_ = name; }

private:
    std::string name_;
    std::shared_ptr<SymbolTable> parent_;
    
    // Type definitions (structs, enums, aliases)
    std::unordered_map<std::string, std::shared_ptr<Omniscript::Value>> types_;
    
    // Variable storage
    std::unordered_map<std::string, std::shared_ptr<Omniscript::Value>> variables_;
    std::unordered_map<std::string, std::shared_ptr<Omniscript::Value>> constants_;
};

#endif
