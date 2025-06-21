#include <omniscript/engine/Statement.h>

class ConstructStructPrototype : 
public NamedStatement, 
public TypedStatement {
public:
    ConstructStructPrototype(const std::string& structName, const std::vector<std::shared_ptr<Statement>>& structBody) :
    body(structBody) {
        setName(structName);
    }
    
    std::string getName() const override { return name; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "Construct Struct : " + name; }
    std::string formatError(const std::string& msg) const override {
        return "Error while constructing struct '" + name + "'.\n" + msg;
    };

    std::vector<std::shared_ptr<Statement>> getBody() const { return body; }
private:
    std::vector<std::shared_ptr<Statement>> body;
};