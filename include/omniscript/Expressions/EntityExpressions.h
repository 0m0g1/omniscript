#pragma once
#include <omniscript/Expression.h>

namespace Omniscript {
struct EnumExpression : public Expression {
    EnumExpression(const std::string& enumName, bool hasLookup = false, bool isEnumClass = false)
        : enumName(enumName), hasLookup(hasLookup), isEnumClass(isEnumClass) {
        name = enumName;
    }
    
    void addEntry(int value, const std::string& valueName, std::shared_ptr<Expression> expression) {
        enumerators[valueName] = value;
        expressionMap[valueName] = expression;  
    }
    
    int get(const std::string& enumeration) const {
        auto it = enumerators.find(enumeration);
        if (it != enumerators.end()) return it->second;
        console.error("Enum '" + enumName + "' does not have an entry " + enumeration);
        return -9999999;
    }

    
    std::string getName(int value) const {
        for (const auto& [name, val] : enumerators)
            if (val == value) return name;
        return "";
    }

    
    std::shared_ptr<Expression> getExpression(const std::string& valueName) const {
        auto it = expressionMap.find(valueName);
        if (it != expressionMap.end()) {
            return it->second;
        }
        console.error("Enum expression for '" + enumName + "' does not have an entry " + valueName);
        return nullptr;
    }

    
    std::shared_ptr<Expression> clone() const override {
        auto copy = std::make_shared<EnumExpression>(enumName, hasLookup, isEnumClass);
        copy->enumerators = enumerators;
        copy->expressionMap = expressionMap;  
        return copy;
    }
    
    std::string toString() const override {
        return (isEnumClass ? "enum class " : "enum ") + enumName;
    }
    
    std::string enumName;
    bool hasLookup = false;
    bool isEnumClass = false;

    std::unordered_map<std::string, int> enumerators;  
    std::unordered_map<std::string, std::shared_ptr<Expression>> expressionMap;  
};
}