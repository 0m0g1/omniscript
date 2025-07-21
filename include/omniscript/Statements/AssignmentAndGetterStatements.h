#pragma once
#include <omniscript/Statement.h>

class Assignment : 
public NamedStatement, 
public TypedStatement,
public ContextAwareStatement {
public:
    bool isStatic = false;
    bool isConstant = false;
    bool isGlobal = true;
    std::shared_ptr<Statement> value;

    void setGlobalVisibilityTo(bool state) {
        isGlobal = state;
    }
    void markAsConstant(bool state = true)  {
        isConstant = state;
    }
    std::shared_ptr<Statement> getValue() { return value; }
    virtual std::string toString() const override { return "Assignment for " + name; }
};

class AssignVariable : public Assignment {
public:
    AssignVariable(const std::string &var, std::shared_ptr<Omniscript::Type> ty, std::shared_ptr<Statement> val, bool isReassign = false)
    : variable(var), isReassign(isReassign) {
        setType(ty);
        this->value = val;
    }

    std::string getName() const override { return variable; }
    void setName(const std::string newVarName) { variable = newVarName; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override {
        return (isConstant ? "AssignConstant" : "AssignVariableStatement") + std::string(": ") + (value ? value->toString() : "null");
    }
    std::string formatError(const std::string& msg) const override {
        return std::string("Error assigning ") + (isConstant ? "constant '" : "variable '") + name + "'.\n" + msg;
    };
    std::shared_ptr<Statement> clone() const override {
        auto clone = std::make_shared<AssignVariable>(variable, type, value->clone(), isReassign);
        clone->isExtern = isExtern;
        clone->libraryPaths = libraryPaths;
        return clone;
    }
    
    bool isVolatile = false;
    bool isExtern = false;

    struct LibraryPaths {
        std::string windowsDynamic;    // .dll
        std::string windowsStatic;     // .lib/.a
        std::string linuxShared;       // .so
        std::string linuxStatic;       // .a
        std::string macosShared;       // .dylib
        std::string macosStatic;       // .a
        std::string genericDynamic;    // fallback dynamic
        std::string genericStatic;     // fallback static
    };

    LibraryPaths libraryPaths;
        
    std::string externName;
    std::string intrinsicName;
    std::string section = "";
private:
    std::string variable;
    bool isReassign;
};


// Variable Retrieval
class GetVariable : 
public NamedStatement,
public TypedStatement,
public Expression {
public:
    explicit GetVariable(const std::string &var) {
        setName(var);
    }

    std::string getName() const override { return name; }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "GetVariable:" + name; }
    std::string formatError(const std::string& msg) const override {
        return "Error in '" + toString() + "'.\n" + msg;
    };
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<GetVariable>(name);
    }

private:
    std::string variable;
};

class GenericAssignment : public NamedStatement {
public:
    GenericAssignment(const std::string &variable, std::shared_ptr<Statement> value) :
        variable(variable), value(value) {}
    std::string getName() const override {return variable;}
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "GenericAssignment"; }

private:
    std::string variable;
    std::shared_ptr<Statement> value;
    std::shared_ptr<Statement> tempValue;
};

class AddressOf : public NamedStatement, public TypedStatement {
public:
    AddressOf(const std::string& value) {
        setName(value);
    }

    std::string getName() const override { return name; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope);
    std::string toString() const override { return "Addressof:" + name; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<AddressOf>(name);
    }
    bool hasSideEffects() override { return true; };
    bool isCompileTimeEvaluatable() override { return false; };
};

class ReferenceTo : public NamedStatement, public TypedStatement {
public:
    ReferenceTo(const std::string& value) {
        setName(value);
    }

    std::string getName() const override { return name; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope);
    std::string toString() const override { return "ReferenceTo: " + name; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<ReferenceTo>(name);
    }
};
