#pragma once
#include <omniscript/Statement.h>

class TypeDeclaration : 
public NamedStatement,
public TypedStatement  {
public:
    TypeDeclaration(const std::string& typeName, std::shared_ptr<Omniscript::Type> type = nullptr) {
        setName(typeName);
        setType(type);
        setRootType(type);
    }

    std::string toString() const override { return "DeclareType:" + name; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
};