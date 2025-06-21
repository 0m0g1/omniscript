#include <omniscript/engine/Statement.h>

class Callable: public NamedStatement {
public:
    std::vector<std::shared_ptr<Statement>> defaultParams;

    Callable(std::vector<std::shared_ptr<Statement>> params = {}) : defaultParams(params) {}
    ~Callable() = default;

    virtual std::string toString() const override { return "CallableStatement"; }
    
    std::vector<std::shared_ptr<ParameterStatement>> cloneParameters() {
        std::vector<std::shared_ptr<ParameterStatement>> clonedParams;

        for (const auto& param : defaultParams) {
            clonedParams.push_back(std::dynamic_pointer_cast<ParameterStatement>(param->clone()));
        }

        return clonedParams;
    }
};

class Call : 
public TypedStatement, 
public NamedStatement,
public ContextAwareStatement {
public:
    Call(const std::string& calleeName, std::vector<std::shared_ptr<Statement>>& arguments) :
    callee(calleeName), args(arguments) {
        setName(calleeName);
    }
    Call(const std::string& objectType, const std::string& instanceName, std::vector<std::shared_ptr<Statement>>& arguments) :
    callee(objectType), instanceName(instanceName), args(arguments) {
        setName(objectType);
    }
    Call(std::shared_ptr<Statement> expr, const std::string& calleeName, std::vector<std::shared_ptr<Statement>>& arguments) :
    expr(expr), callee(calleeName), args(arguments) {
        setName(calleeName);
    }
    Call(std::shared_ptr<Statement> expr, const std::string& objectType, const std::string& instanceName, std::vector<std::shared_ptr<Statement>>& arguments) :
    expr(expr), callee(objectType), instanceName(instanceName), args(arguments) {
        setName(objectType);
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "Call: " + callee; }
    std::string getName() const override { return callee; }
    static bool matchArgumentsToParameters(
        const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& args,
        const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& params,
        SymbolTableType scope
    );
    void setInstanceName(const std::string& name) { instanceName = name; }
    std::string formatError(const std::string& msg) const override {
        return "Error calling '" + callee + (instanceName.empty() ? "'" : "' for instance '" + instanceName + "'") + ".\n" + msg;
    };
    void markAsConstant() {
        isFromConstantAssignment = true;
    }
    static std::string resolveFunctionOverload(
        const std::string& calleeName,
        const std::vector<std::shared_ptr<Statement>>& args,
        const SymbolTableType& scope
    );

    std::shared_ptr<Statement> clone() const override {
        std::vector<std::shared_ptr<Statement>> copiedArgs;
        for (const auto& arg : args) {
            copiedArgs.push_back(arg->clone());
        }

        std::shared_ptr<Statement> copiedExpr = expr ? expr->clone() : nullptr;

        std::shared_ptr<Call> clonedCall;

        if (copiedExpr && !instanceName.empty()) {
            clonedCall = std::make_shared<Call>(copiedExpr, callee, instanceName, copiedArgs);
        } else if (copiedExpr) {
            clonedCall = std::make_shared<Call>(copiedExpr, callee, copiedArgs);
        } else if (!instanceName.empty()) {
            clonedCall = std::make_shared<Call>(callee, instanceName, copiedArgs);
        } else {
            clonedCall = std::make_shared<Call>(callee, copiedArgs);
        }

        clonedCall->isFromAssignment = isFromAssignment;
        if (isFromConstantAssignment) {
            clonedCall->markAsConstant();
        }

        return clonedCall;
    }

    bool isFromAssignment = false;
    private:
        bool isFromConstantAssignment = false;
        std::string callee;
        std::string instanceName;
        std::vector<std::shared_ptr<Statement>> args;
        std::shared_ptr<Statement> expr;
};

class ObjectConstructorStatement : 
public TypedStatement,
public NamedStatement {
private:
    std::shared_ptr<Statement> expr;
    std::string objectType;
    std::string instanceName;
    std::vector<std::shared_ptr<Statement>> constructorArgs;

public:
    ObjectConstructorStatement(
        const std::string& objectType,
        const std::string& instanceName,
        std::vector<std::shared_ptr<Statement>> args = {})
        : objectType(objectType),
            instanceName(instanceName),
            constructorArgs(std::move(args)) {}
    
    ObjectConstructorStatement(
        std::shared_ptr<Statement> expr,
        const std::string& objectType,
        const std::string& instanceName,
        std::vector<std::shared_ptr<Statement>> args = {})
        :   expr(expr),
            objectType(objectType),
            instanceName(instanceName),
            constructorArgs(std::move(args)) {
                this->name = instanceName;
            }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { 
        return "ObjectConstructor(" + objectType + " " + instanceName + ")"; 
    }
    std::string formatError(const std::string& msg) const override {
        return "Error constructing object '" + instanceName + "'.\n" + msg;
    };
    void setInstanceName(const std::string& name) {
        this->name = name;
        instanceName = name; 
    }
};

class ParameterStatement : 
public NamedStatement, 
public TypedStatement,
public ContextAwareStatement {
public:
    bool isVariadic = false;
    bool isConstant;
    std::shared_ptr<Statement> defaultValue;

    ParameterStatement(
        std::string name,
        std::shared_ptr<Statement> defaultValue = nullptr,
        bool isConst = false
    ) : defaultValue(std::move(defaultValue)), isConstant(isConst) {
            setName(name);
        }
    
    ParameterStatement(const ParameterStatement& other)
        : NamedStatement(other), TypedStatement(other),
          defaultValue(other.defaultValue ? other.defaultValue->clone() : nullptr) 
    {
        setPosition(other.getPosition());
    }

    std::string getName() const override { return name; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { 
        return "Parameter: " + (defaultValue? defaultValue->toString() : "no default value"); 
    }
    std::string formatError(const std::string& msg) const override {
        return "Error in parameter '" + name + "'.\n" + msg;
    };
    std::shared_ptr<Statement> getDefaultValue();

    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<ParameterStatement>(*this);
    }
};

class ArgumentStatement :
public NamedStatement, 
public TypedStatement,
public ContextAwareStatement {
public:
    std::string name;
    std::shared_ptr<Statement> value;
    bool isConstant;

    ArgumentStatement(std::string name, std::shared_ptr<Statement> value = nullptr, bool isConstant = false)
        : name(std::move(name)), value(std::move(value)), isConstant(isConstant) {}

    std::string getName() const override { return name; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { 
        return "Argument: " + (value? value->toString() : " no value"); 
    }
    std::string formatError(const std::string& msg) const override {
        return "Error in '" + toString() + "'.\n" + msg;
    };
};