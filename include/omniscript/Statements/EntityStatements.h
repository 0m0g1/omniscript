#pragma once
#include <omniscript/Statements/Statement.h>

namespace Omniscript {

class EnumValue : public NamedStatement, public TypedStatement {
public:
    EnumValue(const std::string& valueName, int& index) :
    valueIndex(index) {
        setName(valueName);
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::string toString() const override {
        return "{" + name + ":" + std::to_string(valueIndex) + "}";
    }
    
    int getIndex() const { return valueIndex; }
    std::string getName() const override { return name; }

private:
    int valueIndex;
};

class EnumConstructor : public NamedStatement {
private:
    std::vector<std::shared_ptr<EnumValue>> values;
    bool hasLookup;
    bool isEnumClass;

public:
    EnumConstructor(
        const std::string& enumName,
        const std::vector<std::shared_ptr<EnumValue>>& values,
        bool hasLookup = false,
        bool isEnumClass = false
    ) : values(values), hasLookup(hasLookup), isEnumClass(isEnumClass) {
        setName(enumName);
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::string toString() const override {
        return "EnumConstructor for " + name + (hasLookup ? " (with lookup)" : "");
    }
    std::string formatError(const std::string& msg) const override {
        return "Error constructing enum '" + name + "'.\n" + msg;
    };
    std::string getName() const override { return name; }
};

} // namespace Omniscript
