#pragma once
#include <omniscript/Statement.h>

namespace Omniscript {
class Access : 
public TypedStatement, 
public Expression, 
public NamedStatement {
protected:
    std::string memberName;
    std::shared_ptr<Statement> assignmentValue = nullptr;
    std::vector<std::shared_ptr<Statement>> arguments;
    bool isCall = false;

public:
    std::shared_ptr<Statement> expr;
    virtual ~Access() = default;

    void setAssignmentValueTo(std::shared_ptr<Statement> newVal = nullptr) {
        assignmentValue = newVal;
    }

    bool isSetter() const {
        return assignmentValue != nullptr;
    }

    const std::shared_ptr<Statement>& getAssignmentValue() const {
        return assignmentValue;
    }

    void setArguments(const std::vector<std::shared_ptr<Statement>>& args) {
        arguments = args;
        isCall = true;
    }

    bool isMethodCall() const {
        return isCall;
    }

    const std::vector<std::shared_ptr<Statement>>& getArguments() const {
        return arguments;
    }

    void setMemberName(const std::string& name) {
        memberName = name;
    }

    std::string getMemberName() const {
        return memberName;
    }

    void verifyMemberAccessibility();

    std::string getFullMemberPath() const {
        return memberName;
    }

    std::string toString() const override {
        return "AccessStatement";
    }
};

class MemberAccess : public Access {
private:
    std::string objectName;
    std::shared_ptr<Statement> object;

    // Helper method declarations
    void validateAccessChain(SymbolTableType scope);
    std::tuple<std::shared_ptr<Expression>, std::string, std::string> 
        resolveBaseExpression(SymbolTableType scope);
    std::tuple<std::shared_ptr<Expression>, std::string, std::string>
        resolveVariableBase(SymbolTableType scope, const std::string& varName);
    std::tuple<std::shared_ptr<Expression>, std::string, std::string>
        resolveChainedAccess(SymbolTableType scope, std::shared_ptr<MemberAccess> memberAcc);
    std::pair<std::shared_ptr<Expression>, std::string>
        findVariableInScope(SymbolTableType scope, const std::string& varName);
    std::string extractTypeName(std::shared_ptr<Type> type);
    std::shared_ptr<UserDefinedType> 
        getUserDefinedType(SymbolTableType scope, const std::string& typeName);
    int findMemberIndex(std::shared_ptr<UserDefinedType> userType);
    std::shared_ptr<Expression> 
        processAssignment(SymbolTableType scope);

public:
    MemberAccess(const std::string& obj, const std::string& member, std::shared_ptr<Statement> assignVal = nullptr);
    MemberAccess(std::shared_ptr<Statement> obj, const std::string& member, std::shared_ptr<Statement> assignVal = nullptr);

    const std::shared_ptr<Statement>& getObject() const;
    std::shared_ptr<Statement> clone() const override;
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override;
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::string toString() const override;
    std::string formatError(const std::string& msg) const override;
};

class Dereference : public Access {
private:
    std::shared_ptr<Statement> pointer;

public:
    Dereference(std::shared_ptr<Statement> ptr, const std::string& member) : pointer(ptr) {
        memberName = member;
        expr = ptr;
    }

    std::shared_ptr<Statement> clone() const override {
        auto cloned = std::make_shared<Dereference>(pointer->clone(), memberName);
        cloned->assignmentValue = assignmentValue ? assignmentValue->clone() : nullptr;
        for (const auto& arg : arguments) {
            cloned->arguments.push_back(arg->clone());
        }
        cloned->isCall = isCall;
        return cloned;
    }

    std::shared_ptr<Expression> express(SymbolTableType scope) override;

    std::string toString() const override {
        std::string base = "(*" + (pointer ? pointer->toString() : "null") + "." + memberName + ")";

        if (isSetter()) {
            return base + " = " + (assignmentValue ? assignmentValue->toString() : "null");
        } else if (isCall) {
            std::string argsStr = "(";
            for (size_t i = 0; i < arguments.size(); ++i) {
                argsStr += arguments[i] ? arguments[i]->toString() : "null";
                if (i + 1 < arguments.size()) argsStr += ", ";
            }
            argsStr += ")";
            return "Call: " + base + argsStr;
        } else {
            return "Get: " + base;
        }
    }
};

class ArrowAccess : public Access {
private:
    std::shared_ptr<Statement> pointer;

    // Helper method declarations
    std::shared_ptr<Expression> resolvePointerExpression(SymbolTableType scope);
    std::shared_ptr<UserDefinedType> validateAndGetPointeeType(
        std::shared_ptr<Type> pointerType);
    int findMemberInType(std::shared_ptr<UserDefinedType> userType);
    std::shared_ptr<Expression> processAssignment(SymbolTableType scope);

public:
    ArrowAccess(std::shared_ptr<Statement> ptr, const std::string& member);
    
    std::shared_ptr<Statement> clone() const override;
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::string toString() const override;
    std::string formatError(const std::string& msg) const override;
};

class IndexAccess : public Access {
private:
    std::shared_ptr<Statement> index;

    // Helper method declarations
    std::shared_ptr<Expression> resolveContainerExpression(SymbolTableType scope);
    std::shared_ptr<Expression> resolveIndexExpression(SymbolTableType scope);
    std::shared_ptr<Type> validateAndGetElementType(
        std::shared_ptr<Type> containerType);
    std::shared_ptr<Expression> processAssignment(SymbolTableType scope);

public:
    IndexAccess(std::shared_ptr<Statement> expr, std::shared_ptr<Statement> index);
    
    std::shared_ptr<Statement> getIndex() const { return index; }
    
    std::shared_ptr<Statement> clone() const override;
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::string toString() const override;
    std::string formatError(const std::string& msg) const override;
};

} // namespace Omniscript
