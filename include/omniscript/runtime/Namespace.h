#pragma once
#include<omniscript/omniscript_pch.h>
#include<omniscript/runtime/object.h>

class Namespace : public Object {
public:
    Namespace(const std::string& name, std::vector<std::shared_ptr<Statement>> body);
    virtual ~Namespace() = default;
    
    void setProperty(std::string name, SymbolTable::ValueType value) override;
    SymbolTable::ValueType getProperty(const std::string& name) const override;
    
    void evaluate(SymbolTable& scope);
    
    std::string toString(int indentLevel = 0) const override;
    
private:
    std::string name;
    std::shared_ptr<SymbolTable> localScope;
    std::vector<std::shared_ptr<Statement>> body;
};