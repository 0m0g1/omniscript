#pragma once
#include<omniscript/omniscript_pch.h>
#include<omniscript/runtime/object.h>

class Enum : public Object {
public:
    Enum(const std::string& name, const std::vector<std::string>& keys, const std::vector<std::shared_ptr<Statement>>& values);
    virtual ~Enum() = default;
    
    void setProperty(std::string name, SymbolTable::ValueType value) override;
    // SymbolTable::ValueType getProperty(const std::string& name) const override;
    
    std::string getPropertyName(int value) const; // New method
    
    std::string toString(int indentLevel = 0) const override;
};