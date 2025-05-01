#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <type_traits>
#include <iostream>

template <typename T, typename TypeT = void>
class SymbolTable : public std::enable_shared_from_this<SymbolTable<T, TypeT>> {
public:
    SymbolTable(std::shared_ptr<SymbolTable<T, TypeT>> parent = nullptr, const std::string& name = "")
        : parent_(std::move(parent)), name_(name) {}

    // ==================== VALUE MANAGEMENT ====================
    void set(const std::string& name, T value) {
        setVariable(name, value);
    }

    void setVariable(const std::string& name, T value) {
        if (overloadables_.count(name)) {
            std::cerr << "Cannot define variable '" << name << "' because an overloaded function exists.\n";
            return;
        }
        if (constants_.count(name)) {
            std::cerr << "Cannot define variable '" << name << "' because a constant exists.\n";
            return;
        }
        variables_[name] = std::move(value);
    }

    void setConstant(const std::string& name, T value) {
        if (overloadables_.count(name)) {
            std::cerr << "Cannot define constant '" << name << "' because an overloaded function exists.\n";
            return;
        }
        if (variables_.count(name)) {
            std::cerr << "Cannot define constant '" << name << "' because a variable exists.\n";
            return;
        }
        if (constants_.count(name)) {
            std::cerr << "Constant '" << name << "' already exists.\n";
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
               (parent_ && parent_->exists(name));
    }

    // ==================== TYPE MANAGEMENT (enabled only if TypeT != void) ====================
    template <typename U = TypeT>
    typename std::enable_if<!std::is_void<U>::value, void>::type
    addType(const std::string& name, U type) {
        types_[name] = std::move(type);
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

    // ==================== SCOPE MANAGEMENT ====================
    std::shared_ptr<SymbolTable<T, TypeT>> createChildScope(const std::string& name) {
        return std::make_shared<SymbolTable<T, TypeT>>(this->shared_from_this(), name);
    }

    std::shared_ptr<SymbolTable<T, TypeT>> getParent() const { return parent_; }

    std::string getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

private:
    std::string name_;
    std::shared_ptr<SymbolTable<T, TypeT>> parent_;

    std::unordered_map<std::string, T> variables_;
    std::unordered_map<std::string, T> constants_;
    std::unordered_map<std::string, std::vector<T>> overloadables_;

    typename std::conditional<std::is_void<TypeT>::value, int, std::unordered_map<std::string, TypeT>>::type types_;
    // Note: 'types_' is an int dummy when TypeT is void, so it won’t take up space.
};

#endif // SYMBOLTABLE_H
