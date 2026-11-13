#pragma once
#include <omniscript/Statements/Statement.h>

namespace Omniscript {

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
    AssignVariable(const std::string &var, std::shared_ptr<Type> ty, std::shared_ptr<Statement> val, bool isReassign = false)
    : variable(var), isReassign(isReassign) {
        setType(ty);
        this->value = val;
    }

    std::string getName() const override { return variable; }
    void setName(const std::string newVarName) { variable = newVarName; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::string toString() const override {
        return (isConstant ? "AssignConstant" : "AssignVariableStatement") + std::string(": ") + (value ? value->toString() : "null");
    }
    std::string formatError(const std::string& msg) const override {
        return std::string("Error assigning ") + (isConstant ? "constant '" : "variable '") + name + "'.\n" + msg;
    };
    std::shared_ptr<Statement> clone() const override {
        auto clone = std::make_shared<AssignVariable>(variable, type, value ? value->clone() : nullptr, isReassign);
        clone->isStatic = isStatic;
        clone->isConstant = isConstant;
        clone->isGlobal = isGlobal;
        clone->isVolatile = isVolatile;
        clone->externName = externName;
        clone->intrinsicName = intrinsicName;
        clone->section = section;
        clone->isExtern = isExtern;
        clone->libraryPaths = libraryPaths;
        clone->setSpan(getSpan());
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
public ASTExpression {
public:
    explicit GetVariable(const std::string &var) {
        setName(var);
    }

    std::string getName() const override { return name; }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "GetVariable:" + name; }
    std::string formatError(const std::string& msg) const override {
        return "Error in '" + toString() + "'.\n" + msg;
    };
    std::shared_ptr<Statement> clone() const override {
        auto clone = std::make_shared<GetVariable>(name);
        clone->setSpan(getSpan());
        return clone;
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
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "GenericAssignment"; }
    std::shared_ptr<Statement> clone() const override {
        auto copy = std::make_shared<GenericAssignment>(variable, value ? value->clone() : nullptr);
        copy->setSpan(getSpan());
        return copy;
    }

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
    std::shared_ptr<Expression> express(SymbolTableType scope);
    std::string toString() const override { return "Addressof:" + name; }
    std::shared_ptr<Statement> clone() const override {
        auto clone = std::make_shared<AddressOf>(name);
        clone->setSpan(getSpan());
        return clone;
    }
    bool hasSideEffects() override { return true; };
    bool isCompileTimeEvaluatable() override { return false; };
};

class ReferenceTo : public NamedStatement, public TypedStatement {
public:
    ReferenceTo(const std::string& value) {
        setName(value);
    }

    ReferenceTo(const std::shared_ptr<Statement>& value) {
        referent = value;
    }

    std::string getName() const override { return name; }
    std::shared_ptr<Expression> express(SymbolTableType scope);

    std::string toString() const override {
        // Show referent when present, so `(ReferenceTo: )` in debug logs becomes
        // `(ReferenceTo: <referent>)` and the bug is visible at a glance.
        if (name.empty() && referent) return "ReferenceTo: <" + referent->toString() + ">";
        return "ReferenceTo: " + name;
    }

    std::shared_ptr<Statement> clone() const override {
        // FIX: preserve referent. Previous version always used the string constructor
        // and dropped `referent`, turning `&this.data[i]` into `&""` after cloning.
        std::shared_ptr<ReferenceTo> copy;
        if (referent) {
            copy = std::make_shared<ReferenceTo>(referent->clone());
        } else {
            copy = std::make_shared<ReferenceTo>(name);
        }
        copy->name = name;
        copy->type = type;
        copy->rootType = rootType;
        copy->setSpan(getSpan());
        return copy;
    }

    std::shared_ptr<Statement> referent = nullptr;
};

} // namespace Omniscript