#pragma once
#include <omniscript/Statements/Statement.h>

namespace Omniscript {

using GenericParamList = std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>>;

class ConstructStructPrototype : 
public NamedStatement, 
public TypedStatement {
public:
    ConstructStructPrototype(const std::string& structName, const std::vector<std::shared_ptr<Statement>>& structBody,
                             const GenericParamList& structTypeParams = {}) :
    body(structBody), typeParams(structTypeParams) {
        setName(structName);
    }
    
    std::string getName() const override { return name; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "Construct Struct : " + name; }
    std::string formatError(const std::string& msg) const override {
        return "Error while constructing struct '" + name + "'.\n" + msg;
    };

    std::vector<std::shared_ptr<Statement>> getBody() const { return body; }

    // Non-empty for `struct List<T> {...}`: this prototype is a template, not a concrete type.
    const GenericParamList& getTypeParams() const { return typeParams; }
    bool isGeneric() const { return !typeParams.empty(); }
private:
    std::vector<std::shared_ptr<Statement>> body;
    GenericParamList typeParams;
};

} // namespace Omniscript