#pragma once
#include <omniscript/Expression.h>

namespace Omniscript {
struct AggregateExpression : public virtual Expression {
    ~AggregateExpression() = default;
    std::string toString() const override { return "Aggregate"; }
};

struct MemberExpression : public Expression {
public:
    std::shared_ptr<Expression> value;
    MemberModifiers modifiers;

    MemberExpression(
        const std::string& name,
        std::shared_ptr<Type> type,
        std::shared_ptr<Expression> value,
        const MemberModifiers& mods = {}
    ) {
        this->name = name;
        this->type = type;
        this->value = value;
        this->modifiers = mods;
    }

    // --- Modifier queries ---
    bool isPublic() const        { return modifiers.access == MemberModifiers::AccessModifier::Public; }
    bool isPrivate() const       { return modifiers.access == MemberModifiers::AccessModifier::Private; }
    bool isProtected() const     { return modifiers.access == MemberModifiers::AccessModifier::Protected; }

    bool isStatic() const        { return modifiers.isStatic; }
    bool isConst() const         { return modifiers.isConst; }
    bool isVirtual() const       { return modifiers.isVirtual; }
    bool isOverride() const      { return modifiers.shouldOverride; }
    bool isFinal() const         { return modifiers.isFinal; }
    bool isConstexpr() const     { return modifiers.isConstexpr; }
    bool isInline() const        { return modifiers.isInline; }
    bool isNoexcept() const      { return modifiers.isNoexcept; }
    bool isPureVirtual() const   { return modifiers.isPureVirtual; }
    bool isExplicit() const      { return modifiers.isExplicit; }
    bool isDeleted() const       { return modifiers.isDeleted; }
    bool isDefault() const       { return modifiers.isDefault; }
    bool isMutable() const       { return modifiers.isMutable; }
    bool isThreadLocal() const   { return modifiers.isThreadLocal; }
    bool isExtern() const        { return modifiers.isExtern; }

    MemberModifiers::AccessModifier getAccess() const {
        return modifiers.access;
    }

    std::string getAccessString() const {
        switch (modifiers.access) {
            case MemberModifiers::AccessModifier::Public: return "public";
            case MemberModifiers::AccessModifier::Protected: return "protected";
            case MemberModifiers::AccessModifier::Private: return "private";
        }
        return "unknown";
    }

    std::string toString() const override {
        return modifiers.toString() + (type ? type->toString() : "unknown") + " " + name + ";";
    }

    std::shared_ptr<Expression> clone() const override {
        // Deep clone the value expression if it exists
        std::shared_ptr<Expression> clonedValue = nullptr;
        if (value) {
            clonedValue = value->clone();
        }
        
        // Deep clone the type if it exists
        std::shared_ptr<Type> clonedType = nullptr;
        if (type) {
            clonedType = type->clone();
        }
        
        // Create new MemberExpression with cloned components
        return std::make_shared<MemberExpression>(
            name,           // string is copied by value
            clonedType,     // cloned type
            clonedValue,    // cloned value expression
            modifiers       // MemberModifiers should be copyable by value
        );
    }
};

struct ClassMemberExpression : public MemberExpression {
    ClassMemberExpression(
        const std::string& name,
        std::shared_ptr<Expression> value,
        const MemberModifiers& modifiers = {}
    )
        : MemberExpression(name, value ? value->getType() : Type::createInvalid(), value, modifiers) {
        if (this->value) {
            this->rootType = this->value->getRootType();
        }
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<ClassMemberExpression>(
            name,
            value ? value->clone() : nullptr,
            modifiers
        );
    }

    std::string toString() const override {
        return "ClassMember(" + name + "): " + modifiers.toString();
    }
};

struct ModuleMemberExpression : public MemberExpression {
public:
    std::shared_ptr<Expression> value;

    ModuleMemberExpression(
        const std::string& name,
        std::shared_ptr<Expression> value,
        const MemberModifiers& modifiers = {}
    )
        : MemberExpression(name, value ? value->getType() : Type::createInvalid(), value, modifiers) {
        if (this->value) {
            this->rootType = this->value->getRootType();
        }
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<ModuleMemberExpression>(
            name,
            value ? value->clone() : nullptr,
            modifiers
        );
    }

    std::string toString() const override {
        return "ModuleMember(" + name + "): " + modifiers.toString();
    }
};

struct ModuleExpression : 
public AggregateExpression {
    std::vector<std::shared_ptr<ModuleMemberExpression>> members;

    ModuleExpression(const std::string& moduleName, const std::vector<std::shared_ptr<ModuleMemberExpression>>& members = {})
        : members(members) {
        this->name = moduleName;
        this->type = Type::createInvalid();  
    }

    std::string toString() const override {
        std::string result = "Module " + name + " {\n";
        for (const auto& member : members) {
            result += "  " + member->toString() + "\n";
        }
        result += "}";
        return result;
    }

    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<ModuleMemberExpression>> clonedMembers;
        for (const auto& member : members) {
            clonedMembers.push_back(std::dynamic_pointer_cast<ModuleMemberExpression>(member->clone()));
        }
        return std::make_shared<ModuleExpression>(name, clonedMembers);
    }

    std::shared_ptr<ModuleMemberExpression> getMember(const std::string& name) {
        for (const auto& member : members) {
            if (member->getName() == name) {
                return member;
            }
        }
        console.error("Member '" + name + "' not found in class '" + this->getName() + "'.");
        return nullptr;
    }

};

struct InstanceExpression : public Expression {
public:
    std::string baseName;
    std::string instanceName;
    std::shared_ptr<Type> instanceType;

    std::vector<std::shared_ptr<MemberExpression>> members;  

    InstanceExpression(
        const std::string& baseName,
        const std::string& instanceName,
        const std::vector<std::shared_ptr<MemberExpression>>& members = {}
    ) : baseName(baseName),
        instanceName(instanceName),
        members(members) {

        this->instanceType = std::make_shared<UserDefinedType>(baseName);
        this->type = instanceType; 
    }

    std::string toString() const override {
        return "Instance<" + baseName + "> named " + instanceName;
    }

    
    std::shared_ptr<Expression> getField(const std::string& name) const {
        for (const auto& member : members) {
            if (member->getType()->getParameterName() == name) {
                return member;
            }
        }
        return nullptr; 
    }

    
    bool setField(const std::string& name, const std::shared_ptr<Expression>& newValue) {
        for (auto& member : members) {
            if (member->getType()->getParameterName() == name) {
                member->value = newValue;  
                return true;
            }
        }
        return false; 
    }

    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<MemberExpression>> clonedMembers;
        for (const auto& member : members) {
            clonedMembers.push_back(std::dynamic_pointer_cast<MemberExpression>(member->clone()));
        }

        return std::make_shared<InstanceExpression>(
            baseName,
            instanceName,
            clonedMembers
        );
    }
};
}