#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <omniscript/omniscript_pch.h>
#include <llvm/IR/Value.h>

class SymbolTable {
public:
    using ValueType = llvm::Value*;

    SymbolTable(std::shared_ptr<SymbolTable> parentScope = nullptr) : parent(parentScope) {}
    
    inline void setParent(std::shared_ptr<SymbolTable> parentScope) {
        if (parentScope) {
            parent = parentScope;
        }
    }

    inline std::shared_ptr<SymbolTable> getParent() {
        return parent;
    }
    
    inline void addMember(const std::string &name, ValueType value) {
        members[name] = value;
    }

    // Local set functions
    inline void set(const std::string &name, ValueType value) {
        variables[name] = value;
    }

    inline void setConstant(const std::string &name, ValueType value) {
        constants[name] = value;
    }

    // Get functions with recursive lookup
    inline ValueType get(const std::string &name) {
        if (variables.find(name) != variables.end()) {
            return variables[name];
        }
        if (constants.find(name) != constants.end()) {
            return constants[name];
        }
        if (parent) {
            return parent->get(name);
        }
        return nullptr;
    }

    inline bool has(const std::string &name) {
        return variables.find(name) != variables.end() || constants.find(name) != constants.end();
    }

    inline bool exists(const std::string &name) {
        if (variables.find(name) != variables.end()) {
            return true;
        }
        if (constants.find(name) != constants.end()) {
            return true;
        }
        if (parent) {
            return parent->exists(name);
        }
        return false;
    }

    inline ValueType getConstant(const std::string &name) {
        if (constants.find(name) != constants.end()) {
            return constants[name];
        }
        if (parent) {
            return parent->getConstant(name);
        }
        return nullptr;
    }

    // Unset function to remove variables
    inline void unset(const std::string &name) {
        variables.erase(name);
        constants.erase(name);
    }

    // Create a new nested scope
    inline std::shared_ptr<SymbolTable> createChildScope() {
        return std::make_shared<SymbolTable>(std::make_shared<SymbolTable>(*this));
    }
    
    inline void addModule(const std::string& modulePath, std::shared_ptr<SymbolTable> module, const std::string& alias = "") {
        // Step 1: Always store the module in the top-most table
        if (parent) {
            parent->addModule(modulePath, module);
            // Step 2: Only store alias in the current table (without propagating it)
            if (!alias.empty()) {
                moduleAliases[alias] = modulePath;
            }
            return;
        }
    
        // Step 3: If the module already exists in the top-most table, do nothing
        if (modules.find(modulePath) != modules.end()) {
            return;
        }
    
        // Step 4: Store the module in the top-most table
        modules[modulePath] = std::move(module);
    }
    

    inline bool moduleExists(const std::string& path) {
        if (parent) {
            return parent->moduleExists(path);
        }
        return modules.find(path) != modules.end();
    }

    inline void setName(const std::string& name) { this->name = name; }

private:
    std::string name;
    std::shared_ptr<SymbolTable> parent = nullptr;
    std::unordered_map<std::string, ValueType> variables; // Stores LLVM values
    std::unordered_map<std::string, ValueType> constants;
    std::unordered_map<std::string, ValueType> members; // Stores LLVM values
    std::unordered_map<std::string, std::shared_ptr<SymbolTable>> modules;
    std::unordered_map<std::string, std::string> moduleAliases;
};

#endif // SYMBOLTABLE_H
