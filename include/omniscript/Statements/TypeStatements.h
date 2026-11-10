#pragma once
#include <omniscript/Statements/Statement.h>

namespace Omniscript {

class TypeDeclaration : 
public NamedStatement,
public TypedStatement  {
public:
    TypeDeclaration(const std::string& typeName, std::shared_ptr<Type> type = nullptr) {
        setName(typeName);
        setType(type);
        setRootType(type);
    }

    std::string toString() const override { return "DeclareType:" + name; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
};

} // namespace Omniscript
