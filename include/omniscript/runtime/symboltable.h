#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H


#include <omniscript/omniscript_pch.h>

// Forward declaration to avoid circular dependency
class Function;
class Object;
class Array;
class Namespace;
struct ClassMemberModifiers;


class SymbolTable {
    public:
        // Define ValueType here so it can be used in the class
        // TODO:: Use malloc and a pointer to a pointer instead of std::variant
        using ValueType = std::variant<
                                    std::string,
                                    std::vector<std::string>,
                                    bool,
                                    char, std::vector<char>,
                                    // std::shared_ptr<Function>,
                                    // std::shared_ptr<Statement>, std::vector<std::shared_ptr<Statement>>,
                                    std::shared_ptr<Object>, std::vector<std::shared_ptr<Object>>,
                                    std::shared_ptr<Array>,
                                    std::shared_ptr<Namespace>,
                                    int, unsigned int, long, unsigned long, long long, 
                                    unsigned long long, float, double, long double, void*, std::nullptr_t,
                                    std::shared_ptr<ClassMemberModifiers>
                                    >;
        using StorageType = std::shared_ptr<ValueType>;
    
        using Numbers = std::variant<int, unsigned int, long, unsigned long, long long, unsigned long long, float, double, long double>;
        using String = std::variant<std::string, const char*>;
        using Primitives = std::variant<
                                        Numbers,
                                        String, std::vector<std::variant<Numbers, String, bool>>,
                                        bool
                                        >;
    
        // Constructor, destructor, etc.
        std::string name;
        void setParent(SymbolTable* parentScope);  // Set the currentScopes parent scope (symbolTable)
        void globalSet(const std::string &name, std::optional<ValueType> value);
        
        void set(const std::string &name, std::optional<ValueType> value = std::nullopt); // Set variables
        std::optional<ValueType> get(const std::string &name);

        void setConstant(const std::string &name, std::optional<ValueType> value); // Set constants
        std::optional<ValueType> getConstant(const std::string &name);
        
        // void addFunction(const std::string &name, const std::shared_ptr<Function> &func);
        // std::shared_ptr<Function> getFunction(const std::string &name); // Return a pointer to a function
        
        // void addGenericFunction(const std::string &name, const std::shared_ptr<Function> &func);
        // std::shared_ptr<Function> getGenericFunction(const std::string &name); // Return a pointer to a function
        
        void addObject(const std::string &name, const std::shared_ptr<Object> &obj);
        std::shared_ptr<Object> getObject(const std::string &name); // Return a pointer to a object

        void unset(const std::string& name);
        
        SymbolTable* parent = nullptr;
        std::unordered_map<std::string, std::optional<ValueType>> variables;
        std::unordered_map<std::string, std::optional<ValueType>> constants;
        // std::unordered_map<std::string, std::shared_ptr<Function>> functions;
        // std::unordered_map<std::string, std::shared_ptr<Function>> genericFunctions;
        std::unordered_map<std::string, std::shared_ptr<Object>> objects;
    private:
        //
    };

#endif
