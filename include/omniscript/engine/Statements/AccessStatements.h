#pragma once
#include <omniscript/engine/Statement.h>

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

public:
    MemberAccess(const std::string& obj, const std::string& member, std::shared_ptr<Statement> assignVal = nullptr) {
        this->objectName = obj;
        this->memberName = member;
        this->name = obj;
        setAssignmentValueTo(assignVal);
    }

    MemberAccess(std::shared_ptr<Statement> obj, const std::string& member, std::shared_ptr<Statement> assignVal = nullptr) {
        this->expr = obj;
        this->object = obj;
        this->memberName = member;
        auto named = std::dynamic_pointer_cast<NamedStatement>(obj);
        if (!named) {
            console.error("The object having members should be named");
        } else {
            this->name = named->getName();
        }
        setAssignmentValueTo(assignVal);
    }

    const std::shared_ptr<Statement>& getObject() const { return object; }

    std::shared_ptr<Statement> clone() const override {
        auto cloned = object
            ? std::make_shared<MemberAccess>(expr->clone(), memberName, assignmentValue ? assignmentValue->clone() : nullptr)
            : std::make_shared<MemberAccess>(objectName, memberName, assignmentValue ? assignmentValue->clone() : nullptr);

        cloned->arguments.reserve(arguments.size());
        for (const auto& arg : arguments) {
            cloned->arguments.push_back(arg->clone());
        }
        cloned->isCall = isCall;
        return cloned;
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override {
        return nullptr; // Evaluation logic to be filled
    }

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

    std::string toString() const override {
        std::string base = object ? "ObjectMember(" + object->toString() + ")" : objectName;
        base += "." + memberName;

        if (isCall) {
            std::string argsStr = "(";
            for (size_t i = 0; i < arguments.size(); ++i) {
                argsStr += arguments[i] ? arguments[i]->toString() : "null";
                if (i + 1 < arguments.size()) argsStr += ", ";
            }
            argsStr += ")";
            return "Call: " + base + argsStr;
        } else if (isSetter()) {
            return "Set: " + base + " = " + (assignmentValue ? assignmentValue->toString() : "null");
        } else {
            return "Get: " + base;
        }
    }
    std::string formatError(const std::string& msg) const override {
        return "Error in '" + toString() + "'.\n" + msg;
    };
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

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

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

public:
    ArrowAccess(std::shared_ptr<Statement> ptr, const std::string& member) : pointer(ptr) {
        memberName = member;
        expr = ptr;
    }

    std::shared_ptr<Statement> clone() const override {
        auto cloned = std::make_shared<ArrowAccess>(pointer->clone(), memberName);
        cloned->assignmentValue = assignmentValue ? assignmentValue->clone() : nullptr;
        for (const auto& arg : arguments) {
            cloned->arguments.push_back(arg->clone());
        }
        cloned->isCall = isCall;
        return cloned;
    }

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

    std::string toString() const override {
        std::string base = "(" + (pointer ? pointer->toString() : "null") + "->" + memberName + ")";

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
    std::string formatError(const std::string& msg) const override {
        return "Error in '" + toString() + "'.\n" + msg;
    };
};


class IndexAccess : public Access {
public:
    IndexAccess(std::shared_ptr<Statement> expr, std::shared_ptr<Statement> index) {
        this->expr = expr;
        this->index = index;
    }

    std::shared_ptr<Statement> index;

    std::shared_ptr<Statement> clone() const override {
        auto cloned = std::make_shared<IndexAccess>(expr->clone(), index->clone());
        cloned->assignmentValue = assignmentValue ? assignmentValue->clone() : nullptr;
        for (const auto& arg : arguments) {
            cloned->arguments.push_back(arg->clone());
        }
        cloned->isCall = isCall;
        return cloned;
    }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override {
        std::string base = "(" + (expr ? expr->toString() : "null") + "[" + (index ? index->toString() : "null") + "])";

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
    std::string formatError(const std::string& msg) const override {
        return "Error in '" + toString() + "'.\n" + msg;
    };
};
