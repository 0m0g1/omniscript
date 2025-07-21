#pragma once
#include <omniscript/Statement.h>

class ClassMember : public Member, public TypedStatement {
public:
    ClassMember(
        const std::string& memberName,
        std::shared_ptr<Omniscript::Type> memberType,
        std::shared_ptr<Statement> defaultValue,
        const MemberModifiers& memberModifiers
    ) : Member(memberName, defaultValue, memberModifiers) {
        setType(memberType);
    }

    std::shared_ptr<Statement> getDefaultValue() const { return value; }

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

    std::string toString() const override {
        return "ClassMember(" + modifiers.toString() + name + ")";
    }
};

class ConstructClassPrototype : public NamedStatement, public TypedStatement {
public:
    ConstructClassPrototype(const std::string& className, const std::vector<std::string>& parentClasses = {}, const std::vector<std::shared_ptr<ClassMember>>& structBody = {}) :
    parentClasses(parentClasses), body(structBody) {
        setName(className);
    }
    
    std::string getName() const override { return name; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "Construct Class: " + name; }
    std::string formatError(const std::string& msg) const override {
        return "Error while construting class '" + name + "'.\n" + msg;
    };

    std::vector<std::shared_ptr<ClassMember>> getBody() const { return body; }
private:
    std::vector<std::string> parentClasses;
    std::vector<std::shared_ptr<ClassMember>> body;
};
