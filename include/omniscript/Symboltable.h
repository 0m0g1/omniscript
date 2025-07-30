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
    ModuleInfo(const std::string& n, const std::string& path, 
               std::shared_ptr<SymbolTable<T, TypeT>> table)
        : name(n), fullPath(path), symbolTable(std::move(table)) {}
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
    
    // Event callback types
    using SymbolCallback = std::function<void(const std::string&, const T&)>;
    using ScopeCallback = std::function<void(const std::string&)>;
    
    // Constructor and destructor
    explicit SymbolTable(SymbolTablePtr parent = nullptr, const std::string& name = "");
    virtual ~SymbolTable() = default;

    // Disable copy constructor and assignment (use shared_ptr)
    SymbolTable(const SymbolTable&) = delete;
    SymbolTable& operator=(const SymbolTable&) = delete;

    // Enable move constructor and assignment
    SymbolTable(SymbolTable&&) = default;
    SymbolTable& operator=(SymbolTable&&) = default;

    // ==================== BASIC VALUE MANAGEMENT ====================
    // Convenience methods for backward compatibility
    void set(const std::string& name, T value, bool isPublic = true);
    T get(const std::string& name) const;
    bool exists(const std::string& name) const;
    
    // Enhanced value management
    bool setVariable(const std::string& name, T value, bool isPublic = true);
    bool setConstant(const std::string& name, T value, bool isPublic = true);
    bool addOverloadable(const std::string& name, T value, bool isPublic = true);
    bool removeSymbol(const std::string& name);
    bool updateVariable(const std::string& name, T newValue);
    
    // Advanced getters
    T getValue(const std::string& name) const;
    T* getPointerToValue(const std::string& name);
    const T* getConstPointerToValue(const std::string& name) const;
    std::vector<T> getOverloads(const std::string& name) const;
    LookupResultType lookup(const std::string& name) const;
    
    // Symbol information
    std::optional<SymbolInfoType> getSymbolInfo(const std::string& name) const;
    std::vector<std::string> getAllSymbolNames(bool includeParent = true) const;
    std::vector<std::string> getPublicSymbolNames() const;
    std::vector<std::string> getPrivateSymbolNames() const;
    
    // ==================== TYPE MANAGEMENT ====================
    template <typename U = TypeT>
    typename std::enable_if<!std::is_void<U>::value, bool>::type
    addType(const std::string& name, U type, bool isPublic = true);

    template <typename U = TypeT>
    typename std::enable_if<!std::is_void<U>::value, U>::type
    getType(const std::string& name) const;

    template <typename U = TypeT>
    typename std::enable_if<!std::is_void<U>::value, bool>::type
    typeExists(const std::string& name) const;
    
    template <typename U = TypeT>
    typename std::enable_if<!std::is_void<U>::value, bool>::type
    removeType(const std::string& name);
    
    template <typename U = TypeT>
    typename std::enable_if<!std::is_void<U>::value, std::vector<std::string>>::type
    getAllTypeNames(bool includeParent = true) const;

    // ==================== SCOPE MANAGEMENT ====================
    SymbolTablePtr createChildScope(const std::string& name = "");
    SymbolTablePtr getParent() const;
    SymbolTablePtr getRoot() const;
    
    // Scope navigation
    SymbolTablePtr findScope(const std::string& name) const;
    std::vector<SymbolTablePtr> getChildScopes() const;
    std::vector<std::string> getScopePath() const;
    size_t getScopeDepth() const;
    
    // Scope properties
    std::string getName() const;
    void setName(const std::string& name);
    std::string getFullPath() const;
    bool isGlobalScope() const;
    bool isChildOf(const SymbolTablePtr& potentialParent) const;
    
    // ==================== MODULE MANAGEMENT ====================
    // Static module registry methods
    static bool defineModule(const std::string& path, SymbolTablePtr module);
    static SymbolTablePtr getModuleByPath(const std::string& path);
    static bool moduleExists(const std::string& path);
    static bool unloadModule(const std::string& path);
    static std::vector<std::string> getAllModulePaths();
    static void clearAllModules();
    
    // Module dependency management
    static bool addModuleDependency(const std::string& modulePath, const std::string& dependencyPath);
    static std::vector<std::string> getModuleDependencies(const std::string& modulePath);
    static bool hasCircularDependency(const std::string& modulePath);
    
    // Local module aliasing
    bool aliasModule(const std::string& alias, const std::string& fullPath);
    bool removeModuleAlias(const std::string& alias);
    SymbolTablePtr getModule(const std::string& alias) const;
    std::string resolveModuleAlias(const std::string& alias) const;
    std::vector<std::string> getModuleAliases() const;
    
    // Module importing and exporting
    bool importSymbol(const std::string& symbolName, const std::string& modulePath, 
                     const std::string& alias = "");
    bool importAllPublicSymbols(const std::string& modulePath, const std::string& prefix = "");
    bool exportSymbol(const std::string& symbolName);
    bool exportAllSymbols();
    std::vector<std::string> getExportedSymbols() const;
    
    // ==================== ADVANCED FEATURES ====================
    // Symbol validation and constraints
    using ValidationFunction = std::function<bool(const std::string&, const T&)>;
    void setValidationFunction(ValidationFunction validator);
    bool validateSymbol(const std::string& name, const T& value) const;
    
    // Symbol observers/listeners
    void addSymbolChangeListener(const std::string& symbolName, SymbolCallback callback);
    void removeSymbolChangeListener(const std::string& symbolName);
    void addScopeChangeListener(ScopeCallback callback);
    void removeScopeChangeListener(ScopeCallback callback);
    
    // Batch operations
    bool setMultipleVariables(const std::unordered_map<std::string, T>& variables, bool isPublic = true);
    bool setMultipleConstants(const std::unordered_map<std::string, T>& constants, bool isPublic = true);
    std::unordered_map<std::string, T> getMultipleValues(const std::vector<std::string>& names) const;
    
    // Symbol table serialization (for debugging/persistence)
    std::string serialize(bool includePrivate = false, int maxDepth = -1) const;
    std::unordered_map<std::string, std::string> getDebugInfo() const;
    
    // Performance and memory management
    void optimize();
    void clearUnusedSymbols();
    size_t getMemoryFootprint() const;
    void reserve(size_t expectedSymbols);
    
    // Thread safety
    void enableThreadSafety(bool enable = true);
    bool isThreadSafe() const;
    
    // Statistics
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
    
    Statistics getStatistics() const;
    
    // ==================== ITERATOR SUPPORT ====================
    class SymbolIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<std::string, T>;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;
        
        SymbolIterator(const SymbolTable* table, bool atEnd = false);
        
        reference operator*() const;
        pointer operator->() const;
        SymbolIterator& operator++();
        SymbolIterator operator++(int);
        bool operator==(const SymbolIterator& other) const;
        bool operator!=(const SymbolIterator& other) const;
        
    private:
        const SymbolTable* table_;
        typename std::unordered_map<std::string, SymbolInfoType>::const_iterator current_;
        bool atEnd_;
        mutable value_type currentPair_;
    };
    
    SymbolIterator begin() const;
    SymbolIterator end() const;
    
    // Range-based for loop support for public symbols only
    class PublicSymbolRange {
    public:
        explicit PublicSymbolRange(const SymbolTable& table) : table_(table) {}
        SymbolIterator begin() const;
        SymbolIterator end() const;
    private:
        const SymbolTable& table_;
    };
    
    PublicSymbolRange publicSymbols() const;

private:
    // ==================== PRIVATE MEMBERS ====================
    std::string name_;
    WeakSymbolTablePtr parent_;
    std::vector<SymbolTablePtr> children_;
    
    // Symbol storage
    std::unordered_map<std::string, SymbolInfoType> symbols_;
    std::unordered_map<std::string, OverloadInfoType> overloads_;
    
    // Type storage (only when TypeT is not void)
    typename std::conditional<
        std::is_void<TypeT>::value, 
        int, 
        std::unordered_map<std::string, TypeT>
    >::type types_;
    
    // Module management
    static std::unordered_map<std::string, ModuleInfoType> globalModules_;
    static std::mutex modulesMutex_;
    std::unordered_map<std::string, std::string> localModuleAliases_;
    std::unordered_set<std::string> exportedSymbols_;
    
    // Advanced features
    ValidationFunction validator_;
    std::unordered_map<std::string, std::vector<SymbolCallback>> symbolListeners_;
    std::vector<ScopeCallback> scopeListeners_;
    
    // Thread safety
    mutable std::shared_mutex mutex_;
    bool threadSafeMode_;
    
    // ==================== PRIVATE METHODS ====================
    // Internal symbol management
    bool setSymbolInternal(const std::string& name, T value, bool isConstant, bool isPublic);
    bool removeSymbolInternal(const std::string& name);
    LookupResultType lookupInternal(const std::string& name, bool searchParent = true) const;
    
    // Event notification
    void notifySymbolChanged(const std::string& name, const T& value);
    void notifyScopeChanged(const std::string& operation);
    
    // Module dependency resolution
    static std::vector<std::string> resolveDependencyOrder(const std::string& modulePath);
    static bool checkCircularDependency(const std::string& modulePath, 
                                       std::unordered_set<std::string>& visited,
                                       std::unordered_set<std::string>& recursionStack);
    
    // Utility methods
    SymbolTablePtr getSharedThis();
    const SymbolTablePtr getSharedThis() const;
    std::string generateScopePath() const;
    void updateChildrenParentReferences();
    
    // Thread safety helpers
    template<typename Func>
    auto withReadLock(Func&& func) const -> decltype(func());
    
    template<typename Func>
    auto withWriteLock(Func&& func) -> decltype(func());
    
    // Serialization helpers
    std::string serializeSymbols(bool includePrivate, int currentDepth, int maxDepth) const;
    std::string serializeTypes(bool includePrivate) const;
};

// ==================== TEMPLATE METHOD IMPLEMENTATIONS ====================
// (Implementations that must be in the header file)

template <typename T, typename TypeT>
template <typename U>
typename std::enable_if<!std::is_void<U>::value, bool>::type
SymbolTable<T, TypeT>::addType(const std::string& name, U type, bool isPublic) {
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

template <typename T, typename TypeT>
template <typename U>
typename std::enable_if<!std::is_void<U>::value, U>::type
SymbolTable<T, TypeT>::getType(const std::string& name) const {
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

template <typename T, typename TypeT>
template <typename U>
typename std::enable_if<!std::is_void<U>::value, bool>::type
SymbolTable<T, TypeT>::typeExists(const std::string& name) const {
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

// Utility functions and type traits
namespace SymbolTableUtils {
    template<typename T>
    struct is_symbol_table : std::false_type {};
    
    template<typename T, typename TypeT>
    struct is_symbol_table<SymbolTable<T, TypeT>> : std::true_type {};
    
    template<typename T>
    constexpr bool is_symbol_table_v = is_symbol_table<T>::value;
    
    // Helper functions for module path resolution
    std::string normalizePath(const std::string& path);
    std::string resolvePath(const std::string& basePath, const std::string& relativePath);
    bool isValidModuleName(const std::string& name);
    std::vector<std::string> splitPath(const std::string& path);
    std::string joinPath(const std::vector<std::string>& components);
}

} // namespace Omniscript
